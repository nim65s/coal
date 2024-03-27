/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011-2014, Willow Garage, Inc.
 *  Copyright (c) 2014-2015, Open Source Robotics Foundation
 *  Copyright (c) 2018-2019, Centre National de la Recherche Scientifique
 *  All rights reserved.
 *  Copyright (c) 2021-2024, INRIA
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Open Source Robotics Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/** \author Jia Pan, Florent Lamiraux */

#ifndef HPP_FCL_NARROWPHASE_H
#define HPP_FCL_NARROWPHASE_H

#include <limits>

#include <hpp/fcl/narrowphase/gjk.h>
#include <hpp/fcl/collision_data.h>
#include <hpp/fcl/narrowphase/narrowphase_defaults.h>
#include <hpp/fcl/logging.h>

namespace hpp {
namespace fcl {

/// @brief collision and distance solver based on GJK algorithm implemented in
/// fcl (rewritten the code from the GJK in bullet)
struct HPP_FCL_DLLAPI GJKSolver {
 public:
  typedef Eigen::Array<FCL_REAL, 1, 2> Array2d;

  //// @brief intersection checking between one shape and a triangle with
  /// transformation
  /// @return true if the shape are colliding.
  /// The variables `gjk_status` and `epa_status` can be used to
  /// check if GJK or EPA ran successfully.
  template <typename S>
  bool shapeTriangleInteraction(const S& s, const Transform3f& tf1,
                                const Vec3f& P1, const Vec3f& P2,
                                const Vec3f& P3, const Transform3f& tf2,
                                FCL_REAL& distance, bool compute_penetration,
                                Vec3f& p1, Vec3f& p2, Vec3f& normal) const {
    // Express everything in frame 1
    const Transform3f tf_1M2(tf1.inverseTimes(tf2));
    TriangleP tri(tf_1M2.transform(P1), tf_1M2.transform(P2),
                  tf_1M2.transform(P3));

    bool relative_transformation_already_computed = true;
    bool gjk_and_epa_ran_successfully =
        runGJKAndEPA(s, tf1, tri, tf_1M2, distance, compute_penetration, p1, p2,
                     normal, relative_transformation_already_computed);
    HPP_FCL_UNUSED_VARIABLE(gjk_and_epa_ran_successfully);
    return (gjk.status == details::GJK::Collision ||
            gjk.status == details::GJK::CollisionWithPenetrationInformation);
  }

  /// @brief distance computation between two shapes.
  /// @return true if no error occured, false otherwise.
  /// The variables `gjk_status` and `epa_status` can be used to
  /// understand the reason of the failure.
  template <typename S1, typename S2>
  bool shapeDistance(const S1& s1, const Transform3f& tf1, const S2& s2,
                     const Transform3f& tf2, FCL_REAL& distance,
                     bool compute_penetration, Vec3f& p1, Vec3f& p2,
                     Vec3f& normal) const {
    bool gjk_and_epa_ran_successfully = runGJKAndEPA(
        s1, tf1, s2, tf2, distance, compute_penetration, p1, p2, normal);
    return gjk_and_epa_ran_successfully;
  }

 protected:
  /// @brief initialize GJK.
  /// This method assumes `minkowski_difference` has been set.
  template <typename S1, typename S2>
  void getGJKInitialGuess(const S1& s1, const S2& s2, Vec3f& guess,
                          support_func_guess_t& support_hint,
                          const Vec3f& default_guess = Vec3f(1, 0, 0)) const {
    switch (gjk_initial_guess) {
      case GJKInitialGuess::DefaultGuess:
        guess = default_guess;
        support_hint.setZero();
        break;
      case GJKInitialGuess::CachedGuess:
        guess = cached_guess;
        support_hint = support_func_cached_guess;
        break;
      case GJKInitialGuess::BoundingVolumeGuess:
        if (s1.aabb_local.volume() < 0 || s2.aabb_local.volume() < 0) {
          HPP_FCL_THROW_PRETTY(
              "computeLocalAABB must have been called on the shapes before "
              "using "
              "GJKInitialGuess::BoundingVolumeGuess.",
              std::logic_error);
        }
        guess.noalias() = s1.aabb_local.center() -
                          (minkowski_difference.oR1 * s2.aabb_local.center() +
                           minkowski_difference.ot1);
        support_hint.setZero();
        break;
      default:
        HPP_FCL_THROW_PRETTY("Wrong initial guess for GJK.", std::logic_error);
    }
    // TODO: use gjk_initial_guess instead
    HPP_FCL_COMPILER_DIAGNOSTIC_PUSH
    HPP_FCL_COMPILER_DIAGNOSTIC_IGNORED_DEPRECECATED_DECLARATIONS
    if (enable_cached_guess) {
      guess = cached_guess;
      support_hint = support_func_cached_guess;
    }
    HPP_FCL_COMPILER_DIAGNOSTIC_POP
  }

  /// @brief Runs the GJK algorithm; if the shapes are in found in collision,
  /// also runs the EPA algorithm.
  /// @return true if no error occured, false otherwise.
  /// @tparam InflateSupportsDuringIterations whether the supports should be
  /// inflated during the iterations of GJK and EPA.
  /// Please leave this default value to `false` unless you know what you are
  /// doing. This template parameter is only used for debugging/testing
  /// purposes. In short, there is no need to take into account the swept sphere
  /// radius when computing supports in the iterations of GJK and EPA. GJK and
  /// EPA will correct the solution once they have converged.
  template <typename S1, typename S2,
            bool InflateSupportsDuringIterations = false>
  bool runGJKAndEPA(
      const S1& s1, const Transform3f& tf1, const S2& s2,
      const Transform3f& tf2, FCL_REAL& distance, bool compute_penetration,
      Vec3f& p1, Vec3f& p2, Vec3f& normal,
      bool relative_transformation_already_computed = false) const {
    bool gjk_and_epa_ran_successfully = true;

    // Reset internal state of GJK algorithm
    if (relative_transformation_already_computed)
      minkowski_difference.set<InflateSupportsDuringIterations>(&s1, &s2);
    else
      minkowski_difference.set<InflateSupportsDuringIterations>(&s1, &s2, tf1,
                                                                tf2);
    gjk.reset(gjk_max_iterations, gjk_tolerance);
    epa.status = details::EPA::Status::DidNotRun;

    // Get initial guess for GJK: default, cached or bounding volume guess
    Vec3f guess;
    support_func_guess_t support_hint;
    getGJKInitialGuess(*minkowski_difference.shapes[0],
                       *minkowski_difference.shapes[1], guess, support_hint);

    gjk.evaluate(minkowski_difference, guess, support_hint);
    HPP_FCL_COMPILER_DIAGNOSTIC_PUSH
    HPP_FCL_COMPILER_DIAGNOSTIC_IGNORED_DEPRECECATED_DECLARATIONS
    if (gjk_initial_guess == GJKInitialGuess::CachedGuess ||
        enable_cached_guess) {
      cached_guess = gjk.getGuessFromSimplex();
      support_func_cached_guess = gjk.support_hint;
    }
    HPP_FCL_COMPILER_DIAGNOSTIC_POP

    const FCL_REAL dummy_precision =
        3 * std::sqrt(std::numeric_limits<FCL_REAL>::epsilon());
    HPP_FCL_UNUSED_VARIABLE(dummy_precision);

    switch (gjk.status) {
      case details::GJK::DidNotRun:
        HPP_FCL_ASSERT(false, "GJK did not run. It should have!",
                       std::logic_error);
        distance = -(std::numeric_limits<FCL_REAL>::max)();
        p1 = p2 = normal =
            Vec3f::Constant(std::numeric_limits<FCL_REAL>::quiet_NaN());
        gjk_and_epa_ran_successfully = false;
        break;
      case details::GJK::Failed:
        //
        // GJK ran out of iterations.
        HPP_FCL_LOG_WARNING("GJK ran out of iterations.");
        GJKExtractWitnessPointsAndNormal(tf1, distance, p1, p2, normal);
        gjk_and_epa_ran_successfully = false;
        break;
      case details::GJK::NoCollisionEarlyStopped:
        //
        // Case where GJK early stopped because the distance was found to be
        // above the `distance_upper_bound`.
        // The two witness points have no meaning.
        GJKEarlyStopExtractWitnessPointsAndNormal(tf1, distance, p1, p2,
                                                  normal);
        HPP_FCL_ASSERT(distance >= gjk.distance_upper_bound - dummy_precision,
                       "The distance should be bigger than GJK's "
                       "`distance_upper_bound`.",
                       std::logic_error);
        gjk_and_epa_ran_successfully = true;
        break;
      case details::GJK::NoCollision:
        //
        // Case where GJK converged and proved that the shapes are not in
        // collision, i.e their distance is above GJK's tolerance (default
        // 1e-6).
        GJKExtractWitnessPointsAndNormal(tf1, distance, p1, p2, normal);
        HPP_FCL_ASSERT(std::abs((p1 - p2).norm() - distance) <=
                           gjk.getTolerance() + dummy_precision,
                       "The distance found by GJK should coincide with the "
                       "distance between the closest points.",
                       std::logic_error);
        gjk_and_epa_ran_successfully = true;
        break;
      //
      // Next are the cases where GJK found the shapes to be in collision, i.e.
      // their distance is below GJK's tolerance (default 1e-6).
      case details::GJK::CollisionWithPenetrationInformation:
        GJKExtractWitnessPointsAndNormal(tf1, distance, p1, p2, normal);
        HPP_FCL_ASSERT(distance <= gjk.getTolerance() + dummy_precision,
                       "The distance found by GJK should be negative or at "
                       "least below GJK's tolerance.",
                       std::logic_error);
        gjk_and_epa_ran_successfully = true;
        break;
      case details::GJK::Collision:
        if (!compute_penetration) {
          // Skip EPA and set the witness points and the normal to nans.
          GJKCollisionExtractWitnessPointsAndNormal(tf1, distance, p1, p2,
                                                    normal);
        } else {
          //
          // GJK was not enough to recover the penetration information.
          // We need to run the EPA algorithm to find the witness points,
          // penetration depth and the normal.

          // Reset EPA algorithm. Potentially allocate memory if
          // `epa_max_face_num` or `epa_max_vertex_num` are bigger than EPA's
          // current storage.
          epa.reset(epa_max_iterations, epa_tolerance);

          // TODO: understand why EPA's performance is so bad on cylinders and
          // cones.
          epa.evaluate(gjk, -guess);

          switch (epa.status) {
            //
            // In the following switch cases, until the "Valid" case,
            // EPA either ran out of iterations, of faces or of vertices.
            // The depth, witness points and the normal are still valid,
            // simply not at the precision of EPA's tolerance.
            // The flag `HPP_FCL_ENABLE_LOGGING` enables feebdack on these
            // cases.
            //
            // TODO: Remove OutOfFaces and OutOfVertices statuses and simply
            // compute the upper bound on max faces and max vertices as a
            // function of the number of iterations.
            case details::EPA::OutOfFaces:
              HPP_FCL_LOG_WARNING("EPA ran out of faces.");
              EPAExtractWitnessPointsAndNormal(tf1, distance, p1, p2, normal);
              gjk_and_epa_ran_successfully = false;
              break;
            case details::EPA::OutOfVertices:
              HPP_FCL_LOG_WARNING("EPA ran out of vertices.");
              EPAExtractWitnessPointsAndNormal(tf1, distance, p1, p2, normal);
              gjk_and_epa_ran_successfully = false;
              break;
            case details::EPA::Failed:
              HPP_FCL_LOG_WARNING("EPA ran out of iterations.");
              EPAExtractWitnessPointsAndNormal(tf1, distance, p1, p2, normal);
              gjk_and_epa_ran_successfully = false;
              break;
            case details::EPA::Valid:
            case details::EPA::AccuracyReached:
              HPP_FCL_ASSERT(
                  -epa.depth <= epa.getTolerance() + dummy_precision,
                  "EPA's penetration distance should be negative (or "
                  "at least below EPA's tolerance).",
                  std::logic_error);
              EPAExtractWitnessPointsAndNormal(tf1, distance, p1, p2, normal);
              break;
            case details::EPA::Degenerated:
              HPP_FCL_LOG_WARNING(
                  "EPA warning: created a polytope with a degenerated face.");
              EPAExtractWitnessPointsAndNormal(tf1, distance, p1, p2, normal);
              gjk_and_epa_ran_successfully = true;
              break;
            case details::EPA::NonConvex:
              HPP_FCL_LOG_WARNING(
                  "EPA warning: EPA got called onto non-convex shapes.");
              EPAExtractWitnessPointsAndNormal(tf1, distance, p1, p2, normal);
              gjk_and_epa_ran_successfully = true;
              break;
            case details::EPA::InvalidHull:
              HPP_FCL_LOG_WARNING("EPA warning: created an invalid polytope.");
              EPAExtractWitnessPointsAndNormal(tf1, distance, p1, p2, normal);
              gjk_and_epa_ran_successfully = true;
              break;
            case details::EPA::DidNotRun:
              HPP_FCL_ASSERT(false, "EPA did not run. It should have!",
                             std::logic_error);
              HPP_FCL_LOG_ERROR("EPA error: did not run. It should have.");
              EPAFailedExtractWitnessPointsAndNormal(tf1, distance, p1, p2,
                                                     normal);
              gjk_and_epa_ran_successfully = false;
              break;
            case details::EPA::FallBack:
              HPP_FCL_ASSERT(
                  false,
                  "EPA went into fallback mode. It should never do that.",
                  std::logic_error);
              HPP_FCL_LOG_ERROR("EPA error: FallBack.");
              EPAFailedExtractWitnessPointsAndNormal(tf1, distance, p1, p2,
                                                     normal);
              gjk_and_epa_ran_successfully = false;
              break;
          }
        }
        break;  // End of case details::GJK::Collision
    }
    return gjk_and_epa_ran_successfully;
  }

  void GJKEarlyStopExtractWitnessPointsAndNormal(const Transform3f& tf1,
                                                 FCL_REAL& distance, Vec3f& p1,
                                                 Vec3f& p2,
                                                 Vec3f& normal) const {
    HPP_FCL_UNUSED_VARIABLE(tf1);
    distance = gjk.distance;
    p1 = p2 = normal =
        Vec3f::Constant(std::numeric_limits<FCL_REAL>::quiet_NaN());
    // If we absolutely want to return some witness points, we could use
    // the following code (or simply merge the early stopped case with the
    // valid case below):
    // gjk.getWitnessPointsAndNormal(minkowski_difference, p1, p2, normal);
    // p1 = tf1.transform(p1);
    // p2 = tf1.transform(p2);
    // normal = tf1.getRotation() * normal;
  }

  void GJKExtractWitnessPointsAndNormal(const Transform3f& tf1,
                                        FCL_REAL& distance, Vec3f& p1,
                                        Vec3f& p2, Vec3f& normal) const {
    // Apart from early stopping, there are two cases where GJK says there is no
    // collision:
    // 1. GJK proved the distance is above its tolerance (default 1e-6).
    // 2. GJK ran out of iterations.
    // In any case, `gjk.ray`'s norm is bigger than GJK's tolerance and thus
    // it can safely be normalized.
    const FCL_REAL dummy_precision =
        3 * std::sqrt(std::numeric_limits<FCL_REAL>::epsilon());
    HPP_FCL_UNUSED_VARIABLE(dummy_precision);

    HPP_FCL_ASSERT(
        gjk.ray.norm() > gjk.getTolerance() - dummy_precision,
        "The norm of GJK's ray should be bigger than GJK's tolerance.",
        std::logic_error);

    distance = gjk.distance;
    // TODO: On degenerated case, the closest points may be non-unique.
    // (i.e. an object face normal is colinear to `gjk.ray`)
    gjk.getWitnessPointsAndNormal(minkowski_difference, p1, p2, normal);
    p1 = tf1.transform(p1);
    p2 = tf1.transform(p2);
    normal = tf1.getRotation() * normal;
  }

  void GJKCollisionExtractWitnessPointsAndNormal(const Transform3f& tf1,
                                                 FCL_REAL& distance, Vec3f& p1,
                                                 Vec3f& p2,
                                                 Vec3f& normal) const {
    HPP_FCL_UNUSED_VARIABLE(tf1);
    const FCL_REAL dummy_precision =
        3 * std::sqrt(std::numeric_limits<FCL_REAL>::epsilon());
    HPP_FCL_UNUSED_VARIABLE(dummy_precision);

    HPP_FCL_ASSERT(gjk.distance <= gjk.getTolerance() + dummy_precision,
                   "The distance should be lower than GJK's tolerance.",
                   std::logic_error);

    distance = gjk.distance;
    p1 = p2 = normal =
        Vec3f::Constant(std::numeric_limits<FCL_REAL>::quiet_NaN());
  }

  void EPAExtractWitnessPointsAndNormal(const Transform3f& tf1,
                                        FCL_REAL& distance, Vec3f& p1,
                                        Vec3f& p2, Vec3f& normal) const {
    distance = (std::min)(0., -epa.depth);
    epa.getWitnessPointsAndNormal(minkowski_difference, p1, p2, normal);
    // The following is very important to understand why EPA can sometimes
    // return a normal that is not colinear to the vector $p_1 - p_2$ when
    // working with tolerances like $\epsilon = 10^{-3}$.
    // It can be resumed with a simple idea:
    //     EPA is an algorithm meant to find the penetration depth and the
    //     normal. It is not meant to find the closest points.
    // Again, the issue here is **not** the normal, it's $p_1$ and $p_2$.
    //
    // More details:
    // We'll denote $S_1$ and $S_2$ the two shapes, $n$ the normal and $p_1$ and
    // $p_2$ the witness points. In theory, when EPA converges to $\epsilon =
    // 0$, the normal and witness points verify the following property (P):
    //   - $p_1 \in \partial \sigma_{S_1}(n)$,
    //   - $p_2 \in \partial \sigma_{S_2}(-n),
    // where $\sigma_{S_1}$ and $\sigma_{S_2}$ are the support functions of
    // $S_1$ and $S_2$. The $\partial \sigma(n)$ simply denotes the support set
    // of the support function in the direction $n$. (Note: I am leaving out the
    // details of frame choice for the support function, to avoid making the
    // mathematical notation too heavy.)
    // --> In practice, EPA converges to $\epsilon > 0$.
    // On polytopes and the likes, this does not change much and the property
    // given above is still valid.
    // --> However, this is very different on curved surfaces, such as
    // ellipsoids, cylinders, cones, capsules etc. For these shapes, converging
    // at $\epsilon = 10^{-6}$ or to $\epsilon = 10^{-3}$ does not change the
    // normal much, but the property (P) given above is no longer valid, which
    // means that the points $p_1$ and $p_2$ do not necessarily belong to the
    // support sets in the direction of $n$ and thus $n$ and $p_1 - p_2$ are not
    // colinear.
    //
    // Do not panic! This is fine.
    // Although the property above is not verified, it's almost verified,
    // meaning that $p_1$ and $p_2$ belong to support sets in directions that
    // are very close to $n$.
    //
    // Solution to compute better $p_1$ and $p_2$:
    // We compute the middle points of the current $p_1$ and $p_2$ and we use
    // the normal and the distance given by EPA to compute the new $p_1$ and
    // $p_2$.
    p1 = tf1.transform(p1);
    p2 = tf1.transform(p2);
    normal = tf1.getRotation() * normal;
  }

  void EPAFailedExtractWitnessPointsAndNormal(const Transform3f& tf1,
                                              FCL_REAL& distance, Vec3f& p1,
                                              Vec3f& p2, Vec3f& normal) const {
    HPP_FCL_UNUSED_VARIABLE(tf1);
    distance = -(std::numeric_limits<FCL_REAL>::max)();
    p1 = p2 = normal =
        Vec3f::Constant(std::numeric_limits<FCL_REAL>::quiet_NaN());
  }

 public:
  HPP_FCL_COMPILER_DIAGNOSTIC_PUSH
  HPP_FCL_COMPILER_DIAGNOSTIC_IGNORED_DEPRECECATED_DECLARATIONS
  /// @brief Default constructor for GJK algorithm
  /// By default, we don't want EPA to allocate memory because
  /// certain functions of the `GJKSolver` class have specializations
  /// which don't use EPA (and/or GJK).
  /// So we give EPA's constructor a max number of iterations of zero.
  /// Only the functions that need EPA will reset the algorithm and allocate
  /// memory if needed.
  GJKSolver()
      : gjk(GJK_DEFAULT_MAX_ITERATIONS, GJK_DEFAULT_TOLERANCE),
        epa(0, EPA_DEFAULT_TOLERANCE) {
    gjk_max_iterations = GJK_DEFAULT_MAX_ITERATIONS;
    gjk_tolerance = GJK_DEFAULT_TOLERANCE;
    epa_max_iterations = EPA_DEFAULT_MAX_ITERATIONS;
    epa_tolerance = EPA_DEFAULT_TOLERANCE;

    gjk_initial_guess = GJKInitialGuess::DefaultGuess;
    enable_cached_guess = false;  // TODO: use gjk_initial_guess instead
    cached_guess = Vec3f(1, 0, 0);
    support_func_cached_guess = support_func_guess_t::Zero();

    // Default settings for GJK algorithm
    gjk.gjk_variant = GJKVariant::DefaultGJK;
    gjk.convergence_criterion = GJKConvergenceCriterion::Default;
    gjk.convergence_criterion_type = GJKConvergenceCriterionType::Relative;
  }

  /// @brief Constructor from a DistanceRequest
  ///
  /// \param[in] request DistanceRequest input
  ///
  /// See the default constructor; by default, we don't want
  /// EPA to allocate memory so we call EPA's constructor with 0 max
  /// number of iterations.
  explicit GJKSolver(const DistanceRequest& request)
      : gjk(request.gjk_max_iterations, request.gjk_tolerance),
        epa(0, request.epa_tolerance) {
    cached_guess = Vec3f(1, 0, 0);
    support_func_cached_guess = support_func_guess_t::Zero();

    set(request);
  }

  /// @brief setter from a DistanceRequest
  ///
  /// \param[in] request DistanceRequest input
  ///
  void set(const DistanceRequest& request) {
    // ---------------------
    // GJK settings
    gjk_initial_guess = request.gjk_initial_guess;
    enable_cached_guess = request.enable_cached_gjk_guess;
    if (gjk_initial_guess == GJKInitialGuess::CachedGuess ||
        enable_cached_guess) {
      cached_guess = request.cached_gjk_guess;
      support_func_cached_guess = request.cached_support_func_guess;
    }
    gjk_max_iterations = request.gjk_max_iterations;
    gjk_tolerance = request.gjk_tolerance;
    // For distance computation, we don't want GJK to early stop
    gjk.setDistanceEarlyBreak((std::numeric_limits<FCL_REAL>::max)());
    gjk.gjk_variant = request.gjk_variant;
    gjk.convergence_criterion = request.gjk_convergence_criterion;
    gjk.convergence_criterion_type = request.gjk_convergence_criterion_type;
    gjk.status = details::GJK::Status::DidNotRun;

    // ---------------------
    // EPA settings
    epa_max_iterations = request.epa_max_iterations;
    epa_tolerance = request.epa_tolerance;
    epa.status = details::EPA::Status::DidNotRun;
  }

  /// @brief Constructor from a CollisionRequest
  ///
  /// \param[in] request CollisionRequest input
  ///
  /// See the default constructor; by default, we don't want
  /// EPA to allocate memory so we call EPA's constructor with 0 max
  /// number of iterations.
  explicit GJKSolver(const CollisionRequest& request)
      : gjk(request.gjk_max_iterations, request.gjk_tolerance),
        epa(0, request.epa_tolerance) {
    cached_guess = Vec3f(1, 0, 0);
    support_func_cached_guess = support_func_guess_t::Zero();

    set(request);
  }

  /// @brief setter from a CollisionRequest
  ///
  /// \param[in] request CollisionRequest input
  ///
  void set(const CollisionRequest& request) {
    // ---------------------
    // GJK settings
    gjk_initial_guess = request.gjk_initial_guess;
    // TODO: use gjk_initial_guess instead
    enable_cached_guess = request.enable_cached_gjk_guess;
    if (gjk_initial_guess == GJKInitialGuess::CachedGuess ||
        enable_cached_guess) {
      cached_guess = request.cached_gjk_guess;
      support_func_cached_guess = request.cached_support_func_guess;
    }
    gjk_tolerance = request.gjk_tolerance;
    gjk_max_iterations = request.gjk_max_iterations;
    // The distance upper bound should be at least greater to the requested
    // security margin. Otherwise, we will likely miss some collisions.
    const double distance_upper_bound = (std::max)(
        0., (std::max)(request.distance_upper_bound, request.security_margin));
    gjk.setDistanceEarlyBreak(distance_upper_bound);
    gjk.gjk_variant = request.gjk_variant;
    gjk.convergence_criterion = request.gjk_convergence_criterion;
    gjk.convergence_criterion_type = request.gjk_convergence_criterion_type;

    // ---------------------
    // EPA settings
    epa_max_iterations = request.epa_max_iterations;
    epa_tolerance = request.epa_tolerance;

    // ---------------------
    // Reset GJK and EPA status
    gjk.status = details::GJK::Status::DidNotRun;
    epa.status = details::EPA::Status::DidNotRun;
  }

  /// @brief Copy constructor
  GJKSolver(const GJKSolver& other) = default;

  HPP_FCL_COMPILER_DIAGNOSTIC_PUSH
  HPP_FCL_COMPILER_DIAGNOSTIC_IGNORED_DEPRECECATED_DECLARATIONS
  bool operator==(const GJKSolver& other) const {
    return enable_cached_guess ==
               other.enable_cached_guess &&  // TODO: use gjk_initial_guess
                                             // instead
           cached_guess == other.cached_guess &&
           gjk_max_iterations == other.gjk_max_iterations &&
           gjk_tolerance == other.gjk_tolerance &&
           epa_max_iterations == other.epa_max_iterations &&
           epa_tolerance == other.epa_tolerance &&
           support_func_cached_guess == other.support_func_cached_guess &&
           gjk_initial_guess == other.gjk_initial_guess;
  }
  HPP_FCL_COMPILER_DIAGNOSTIC_POP

  bool operator!=(const GJKSolver& other) const { return !(*this == other); }

  /// @brief Whether smart guess can be provided
  /// @Deprecated Use gjk_initial_guess instead
  HPP_FCL_DEPRECATED_MESSAGE(Use gjk_initial_guess instead)
  bool enable_cached_guess;

  /// @brief smart guess
  mutable Vec3f cached_guess;

  /// @brief smart guess for the support function
  mutable support_func_guess_t support_func_cached_guess;

  /// @brief which warm start to use for GJK
  GJKInitialGuess gjk_initial_guess;

  /// @brief maximum number of iterations of GJK
  size_t gjk_max_iterations;

  /// @brief tolerance of GJK
  FCL_REAL gjk_tolerance;

  /// @brief maximum number of iterations of EPA
  size_t epa_max_iterations;

  /// @brief tolerance of EPA
  FCL_REAL epa_tolerance;

  /// @brief GJK algorithm
  mutable details::GJK gjk;

  /// @brief EPA algorithm
  mutable details::EPA epa;

  /// @brief Minkowski difference used by GJK and EPA algorithms
  mutable details::MinkowskiDiff minkowski_difference;

 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

template <>
HPP_FCL_DLLAPI bool GJKSolver::shapeTriangleInteraction(
    const Sphere& s, const Transform3f& tf1, const Vec3f& P1, const Vec3f& P2,
    const Vec3f& P3, const Transform3f& tf2, FCL_REAL& distance,
    bool compute_penetration, Vec3f& p1, Vec3f& p2, Vec3f& normal) const;

template <>
HPP_FCL_DLLAPI bool GJKSolver::shapeTriangleInteraction(
    const Halfspace& s, const Transform3f& tf1, const Vec3f& P1,
    const Vec3f& P2, const Vec3f& P3, const Transform3f& tf2,
    FCL_REAL& distance, bool compute_penetration, Vec3f& p1, Vec3f& p2,
    Vec3f& normal) const;

template <>
HPP_FCL_DLLAPI bool GJKSolver::shapeTriangleInteraction(
    const Plane& s, const Transform3f& tf1, const Vec3f& P1, const Vec3f& P2,
    const Vec3f& P3, const Transform3f& tf2, FCL_REAL& distance,
    bool compute_penetration, Vec3f& p1, Vec3f& p2, Vec3f& normal) const;

#define SHAPE_DISTANCE_SPECIALIZATION_BASE(S1, S2)                      \
  template <>                                                           \
  HPP_FCL_DLLAPI bool GJKSolver::shapeDistance<S1, S2>(                 \
      const S1& s1, const Transform3f& tf1, const S2& s2,               \
      const Transform3f& tf2, FCL_REAL& dist, bool compute_penetration, \
      Vec3f& p1, Vec3f& p2, Vec3f& normal) const

#define SHAPE_DISTANCE_SPECIALIZATION(S1, S2) \
  SHAPE_DISTANCE_SPECIALIZATION_BASE(S1, S2); \
  SHAPE_DISTANCE_SPECIALIZATION_BASE(S2, S1)

SHAPE_DISTANCE_SPECIALIZATION(Sphere, Capsule);
SHAPE_DISTANCE_SPECIALIZATION(Sphere, Box);
SHAPE_DISTANCE_SPECIALIZATION(Sphere, Cylinder);
SHAPE_DISTANCE_SPECIALIZATION_BASE(Sphere, Sphere);
SHAPE_DISTANCE_SPECIALIZATION_BASE(Capsule, Capsule);
SHAPE_DISTANCE_SPECIALIZATION_BASE(TriangleP, TriangleP);

#undef SHAPE_DISTANCE_SPECIALIZATION
#undef SHAPE_DISTANCE_SPECIALIZATION_BASE

#if !(__cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1600))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc99-extensions"
#endif

/// \name Shape triangle interaction specializations
/// \{

// param doc is the doxygen detailled description (should be enclosed in /** */
// and contain no dot for some obscure reasons).
#define HPP_FCL_DECLARE_SHAPE_TRIANGLE(Shape, doc)                        \
  /** @brief Fast implementation for Shape-Triangle interaction. */       \
  doc template <>                                                         \
  HPP_FCL_DLLAPI bool GJKSolver::shapeTriangleInteraction<Shape>(         \
      const Shape& s, const Transform3f& tf1, const Vec3f& P1,            \
      const Vec3f& P2, const Vec3f& P3, const Transform3f& tf2,           \
      FCL_REAL& distance, bool compute_penetration, Vec3f& p1, Vec3f& p2, \
      Vec3f& normal) const

HPP_FCL_DECLARE_SHAPE_TRIANGLE(Sphere, );
HPP_FCL_DECLARE_SHAPE_TRIANGLE(Halfspace, );
HPP_FCL_DECLARE_SHAPE_TRIANGLE(Plane, );

#undef HPP_FCL_DECLARE_SHAPE_TRIANGLE

/// \}

/// \name Shape distance specializations
/// \{

// param doc is the doxygen detailled description (should be enclosed in /** */
// and contain no dot for some obscure reasons).
#define HPP_FCL_DECLARE_SHAPE_DISTANCE(Shape1, Shape2, doc)           \
  /** @brief Fast implementation for Shape1-Shape2 distance. */       \
  doc template <>                                                     \
  bool HPP_FCL_DLLAPI GJKSolver::shapeDistance<Shape1, Shape2>(       \
      const Shape1& s1, const Transform3f& tf1, const Shape2& s2,     \
      const Transform3f& tf2, FCL_REAL& dist, bool compute_collision, \
      Vec3f& p1, Vec3f& p2, Vec3f& normal) const
#define HPP_FCL_DECLARE_SHAPE_DISTANCE_SELF(Shape, doc) \
  HPP_FCL_DECLARE_SHAPE_DISTANCE(Shape, Shape, doc)
#define HPP_FCL_DECLARE_SHAPE_DISTANCE_PAIR(Shape1, Shape2, doc) \
  HPP_FCL_DECLARE_SHAPE_DISTANCE(Shape1, Shape2, doc);           \
  HPP_FCL_DECLARE_SHAPE_DISTANCE(Shape2, Shape1, doc)

HPP_FCL_DECLARE_SHAPE_DISTANCE_PAIR(Sphere, Box, );
HPP_FCL_DECLARE_SHAPE_DISTANCE_PAIR(Sphere, Capsule, );
HPP_FCL_DECLARE_SHAPE_DISTANCE_PAIR(Sphere, Cylinder, );
HPP_FCL_DECLARE_SHAPE_DISTANCE_SELF(Sphere, );

HPP_FCL_DECLARE_SHAPE_DISTANCE_SELF(
    Capsule,
    /** Closest points are based on two line-segments. */
);

HPP_FCL_DECLARE_SHAPE_DISTANCE_SELF(
    TriangleP,
    /** Do not run EPA algorithm to compute penetration depth. Use a dedicated
       method. */
);

#undef HPP_FCL_DECLARE_SHAPE_DISTANCE
#undef HPP_FCL_DECLARE_SHAPE_DISTANCE_SELF
#undef HPP_FCL_DECLARE_SHAPE_DISTANCE_PAIR

/// \}
#if !(__cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1600))
#pragma GCC diagnostic pop
#endif
}  // namespace fcl

}  // namespace hpp

#endif
