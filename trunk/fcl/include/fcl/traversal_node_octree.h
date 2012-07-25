/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011, Willow Garage, Inc.
 *  All rights reserved.
 *
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
 *   * Neither the name of Willow Garage, Inc. nor the names of its
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

/** \author Jia Pan */

#ifndef FCL_TRAVERSAL_NODE_OCTREE_H
#define FCL_TRAVERSAL_NODE_OCTREE_H

#include "fcl/collision_data.h"
#include "fcl/traversal_node_base.h"
#include "fcl/narrowphase/narrowphase.h"
#include "fcl/geometric_shapes_utility.h"
#include "fcl/octree.h"
#include "fcl/BVH_model.h"

namespace fcl
{


template<typename NarrowPhaseSolver>
class OcTreeSolver
{
private:
  const NarrowPhaseSolver* solver;

  mutable const CollisionRequest* crequest;
  mutable const DistanceRequest* drequest;

  mutable CollisionResult* cresult;
  mutable DistanceResult* dresult;

public:
  OcTreeSolver(const NarrowPhaseSolver* solver_) : solver(solver_),
                                                   crequest(NULL),
                                                   drequest(NULL),
                                                   cresult(NULL),
                                                   dresult(NULL)
  {
  }

  void OcTreeIntersect(const OcTree* tree1, const OcTree* tree2,
                       const SimpleTransform& tf1, const SimpleTransform& tf2,
                       const CollisionRequest& request_,
                       CollisionResult& result_) const
  {
    crequest = &request_;
    cresult = &result_;
    
    OcTreeIntersectRecurse(tree1, tree1->getRoot(), tree1->getRootBV(), 
                           tree2, tree2->getRoot(), tree2->getRootBV(), 
                           tf1, tf2);
  }


  FCL_REAL OcTreeDistance(const OcTree* tree1, const OcTree* tree2,
                          const SimpleTransform& tf1, const SimpleTransform& tf2) const
  {
    FCL_REAL min_dist = std::numeric_limits<FCL_REAL>::max();

    OcTreeDistanceRecurse(tree1, tree1->getRoot(), tree1->getRootBV(), 
                          tree2, tree2->getRoot(), tree2->getRootBV(),
                          tf1, tf2, min_dist);

    return min_dist;
  }

  template<typename BV>
  void OcTreeMeshIntersect(const OcTree* tree1, const BVHModel<BV>* tree2,
                           const SimpleTransform& tf1, const SimpleTransform& tf2,
                           const CollisionRequest& request_,
                           CollisionResult& result_) const
  {
    crequest = &request_;
    cresult = &result_;

    OcTreeMeshIntersectRecurse(tree1, tree1->getRoot(), tree1->getRootBV(),
                               tree2, 0,
                               tf1, tf2);
  }

  template<typename BV>
  FCL_REAL OcTreeMeshDistance(const OcTree* tree1, const BVHModel<BV>* tree2,
                              const SimpleTransform& tf1, const SimpleTransform& tf2) const
  {
    FCL_REAL min_dist = std::numeric_limits<FCL_REAL>::max();

    OcTreeMeshDistanceRecurse(tree1, tree1->getRoot(), tree1->getRootBV(),
                              tree2, 0,
                              tf1, tf2, min_dist);
    
    return min_dist;
  }


  template<typename BV>
  void MeshOcTreeIntersect(const BVHModel<BV>* tree1, const OcTree* tree2,
                           const SimpleTransform& tf1, const SimpleTransform& tf2,
                           const CollisionRequest& request_,
                           CollisionResult& result_) const
  
  {
    crequest = &request_;
    cresult = &result_;

    OcTreeMeshIntersectRecurse(tree2, tree2->getRoot(), tree2->getRootBV(),
                               tree1, 0,
                               tf2, tf1);
  }

  
  template<typename BV>
  FCL_REAL MeshOcTreeDistance(const BVHModel<BV>* tree1, const OcTree* tree2,
                              const SimpleTransform& tf1, const SimpleTransform& tf2) const
  {
    FCL_REAL min_dist = std::numeric_limits<FCL_REAL>::max();

    OcTreeMeshDistanceRecurse(tree1, 0,
                              tree2, tree2->getRoot(), tree2->getRootBV(),
                              tf1, tf2, min_dist);
    
    return min_dist;
  }

  template<typename S>
  void OcTreeShapeIntersect(const OcTree* tree, const S& s,
                            const SimpleTransform& tf1, const SimpleTransform& tf2,
                            const CollisionRequest& request_,
                            CollisionResult& result_) const
  {
    crequest = &request_;
    cresult = &result_;

    AABB bv2;
    computeBV<AABB>(s, SimpleTransform(), bv2);
    OBB obb2;
    convertBV(bv2, tf2, obb2);
    OcTreeShapeIntersectRecurse(tree, tree->getRoot(), tree->getRootBV(),
                                s, obb2,
                                tf1, tf2);
    
  }

  template<typename S>
  void ShapeOcTreeIntersect(const S& s, const OcTree* tree,
                            const SimpleTransform& tf1, const SimpleTransform& tf2,
                            const CollisionRequest& request_,
                            CollisionResult& result_) const
  {
    crequest = &request_;
    cresult = &result_;

    AABB bv1;
    computeBV<AABB>(s, SimpleTransform(), bv1);
    OBB obb1;
    convertBV(bv1, tf1, obb1);
    OcTreeShapeIntersectRecurse(tree, tree->getRoot(), tree->getRootBV(),
                                s, obb1,
                                tf2, tf1);
  }

  template<typename S>
  FCL_REAL OcTreeShapeDistance(const OcTree* tree, const S& s,
                               const SimpleTransform& tf1, const SimpleTransform& tf2) const
  {
    AABB aabb2;
    computeBV<AABB>(s, tf2, aabb2);
    FCL_REAL min_dist = std::numeric_limits<FCL_REAL>::max();
    
    OcTreeShapeDistanceRecurse(tree, tree->getRoot(), tree->getRootBV(),
                               s, aabb2,
                               tf1, tf2, min_dist);
    return min_dist;
  }

  template<typename S>
  FCL_REAL ShapeOcTreeDistance(const S& s, const OcTree* tree,
                               const SimpleTransform& tf1, const SimpleTransform& tf2) const
  {
    AABB aabb1;
    computeBV<AABB>(s, tf1, aabb1);
    FCL_REAL min_dist = std::numeric_limits<FCL_REAL>::max();

    OcTreeShapeDistanceRecurse(tree, tree->getRoot(), tree->getRootBV(),
                               s, aabb1,
                               tf2, tf1, min_dist);

    return min_dist;
  }
  

private:
  template<typename S>
  bool OcTreeShapeDistanceRecurse(const OcTree* tree1, const OcTree::OcTreeNode* root1, const AABB& bv1,
                                  const S& s, const AABB& aabb2,
                                  const SimpleTransform& tf1, const SimpleTransform& tf2,
                                  FCL_REAL& min_dist) const
  {
    if(!root1->hasChildren())
    {
      if(tree1->isNodeOccupied(root1))
      {
        Box box;
        SimpleTransform box_tf;
        constructBox(bv1, tf1, box, box_tf);
 
        FCL_REAL dist;
        solver->shapeDistance(box, box_tf, s, tf2, &dist);
        if(dist < min_dist) min_dist = dist;
        
        return (min_dist <= 0);
      }
      else
        return false;
    }

    if(!tree1->isNodeOccupied(root1)) return false;
    
    for(unsigned int i = 0; i < 8; ++i)
    {
      if(root1->childExists(i))
      {
        const OcTree::OcTreeNode* child = root1->getChild(i);
        AABB child_bv;
        computeChildBV(bv1, i, child_bv);
        
        AABB aabb1;
        convertBV(child_bv, tf1, aabb1);
        FCL_REAL d = aabb1.distance(aabb2);
        if(d < min_dist)
        {
          if(OcTreeShapeDistanceRecurse(tree1, child, child_bv, s, aabb2, tf1, tf2, min_dist))
            return true;
        }        
      }
    }

    return false;
  }

  template<typename S>
  bool OcTreeShapeIntersectRecurse(const OcTree* tree1, const OcTree::OcTreeNode* root1, const AABB& bv1,
                                   const S& s, const OBB& obb2,
                                   const SimpleTransform& tf1, const SimpleTransform& tf2) const
  {
    if(!root1->hasChildren())
    {
      if(tree1->isNodeOccupied(root1))
      {
        OBB obb1;
        convertBV(bv1, tf1, obb1);
        if(obb1.overlap(obb2))
        {
          Box box;
          SimpleTransform box_tf;
          constructBox(bv1, tf1, box, box_tf);

          bool is_intersect = false;
          
          if(!crequest->enable_contact)
          {
            if(solver->shapeIntersect(box, box_tf, s, tf2, NULL, NULL, NULL))
            {
              is_intersect = true;
              if(cresult->numContacts() < crequest->num_max_contacts)
                cresult->addContact(Contact(tree1, &s, root1 - tree1->getRoot(), Contact::NONE));
            }
          }
          else
          {
            Vec3f contact;
            FCL_REAL depth;
            Vec3f normal;

            if(solver->shapeIntersect(box, box_tf, s, tf2, &contact, &depth, &normal))
            {
              is_intersect = true;
              if(cresult->numContacts() < crequest->num_max_contacts)
                cresult->addContact(Contact(tree1, &s, root1 - tree1->getRoot(), Contact::NONE, contact, normal, depth));
            }
          }
          
          return ((cresult->numContacts() >= crequest->num_max_contacts) && !crequest->exhaustive);        
        }
        else return false;
      }
      else
        return false;
    }

    OBB obb1;
    convertBV(bv1, tf1, obb1);
    if(!tree1->isNodeOccupied(root1) || !obb1.overlap(obb2)) return false;

    for(unsigned int i = 0; i < 8; ++i)
    {
      if(root1->childExists(i))
      {
        const OcTree::OcTreeNode* child = root1->getChild(i);
        AABB child_bv;
        computeChildBV(bv1, i, child_bv);
        
        if(OcTreeShapeIntersectRecurse(tree1, child, child_bv, s, obb2, tf1, tf2))
          return true;
      }
    }

    return false;    
  }

  template<typename BV>
  bool OcTreeMeshDistanceRecurse(const OcTree* tree1, const OcTree::OcTreeNode* root1, const AABB& bv1,
                                 const BVHModel<BV>* tree2, int root2,
                                 const SimpleTransform& tf1, const SimpleTransform& tf2, FCL_REAL& min_dist) const
  {
    if(!root1->hasChildren() && tree2->getBV(root2).isLeaf())
    {
      if(tree1->isNodeOccupied(root1))
      {
        Box box;
        SimpleTransform box_tf;
        constructBox(bv1, tf1, box, box_tf);

        int primitive_id = tree2->getBV(root2).primitiveId();
        const Triangle& tri_id = tree2->tri_indices[primitive_id];
        const Vec3f& p1 = tree2->vertices[tri_id[0]];
        const Vec3f& p2 = tree2->vertices[tri_id[1]];
        const Vec3f& p3 = tree2->vertices[tri_id[2]];
        
        FCL_REAL dist;
        solver->shapeTriangleDistance(box, box_tf, p1, p2, p3, tf2, &dist);
        if(dist < min_dist) min_dist = dist;
        return (min_dist <= 0);
      }
      else
        return false;
    }

    if(!tree1->isNodeOccupied(root1)) return false;

    if(tree2->getBV(root2).isLeaf() || (root1->hasChildren() && (bv1.size() > tree2->getBV(root2).bv.size())))
    {
      for(unsigned int i = 0; i < 8; ++i)
      {
        if(root1->childExists(i))
        {
          const OcTree::OcTreeNode* child = root1->getChild(i);
          AABB child_bv;
          computeChildBV(bv1, i, child_bv);

          FCL_REAL d;
          AABB aabb1, aabb2;
          convertBV(child_bv, tf1, aabb1);
          convertBV(tree2->getBV(root2).bv, tf2, aabb2);
          d = aabb1.distance(aabb2);
          
          if(d < min_dist)
          {
            if(OcTreeMeshDistanceRecurse(tree1, child, child_bv, tree2, root2, tf1, tf2, min_dist))
              return true;
          }
        }
      }
    }
    else
    {
      FCL_REAL d;
      AABB aabb1, aabb2;
      convertBV(bv1, tf1, aabb1);
      int child = tree2->getBV(root2).leftChild();
      convertBV(tree2->getBV(child).bv, tf2, aabb2);
      d = aabb1.distance(aabb2);

      if(d < min_dist)
      {
        if(OcTreeMeshDistanceRecurse(tree1, root1, bv1, tree2, child, tf1, tf2, min_dist))
          return true;
      }

      child = tree2->getBV(root2).rightChild();
      convertBV(tree2->getBV(child).bv, tf2, aabb2);
      d = aabb1.distance(aabb2);
      
      if(d < min_dist)
      {
        if(OcTreeMeshDistanceRecurse(tree1, root1, bv1, tree2, child, tf1, tf2, min_dist))
          return true;      
      }
    }

    return false;
  }


  template<typename BV>
  bool OcTreeMeshIntersectRecurse(const OcTree* tree1, const OcTree::OcTreeNode* root1, const AABB& bv1,
                                  const BVHModel<BV>* tree2, int root2,
                                  const SimpleTransform& tf1, const SimpleTransform& tf2) const
  {
    if(!root1->hasChildren() && tree2->getBV(root2).isLeaf())
    {
      if(tree1->isNodeOccupied(root1))
      {
        OBB obb1, obb2;
        convertBV(bv1, tf1, obb1);
        convertBV(tree2->getBV(root2).bv, tf2, obb2);
        if(obb1.overlap(obb2))
        {
          Box box;
          SimpleTransform box_tf;
          constructBox(bv1, tf1, box, box_tf);

          int primitive_id = tree2->getBV(root2).primitiveId();
          const Triangle& tri_id = tree2->tri_indices[primitive_id];
          const Vec3f& p1 = tree2->vertices[tri_id[0]];
          const Vec3f& p2 = tree2->vertices[tri_id[1]];
          const Vec3f& p3 = tree2->vertices[tri_id[2]];
        
          if(!crequest->enable_contact)
          {
            if(solver->shapeTriangleIntersect(box, box_tf, p1, p2, p3, tf2, NULL, NULL, NULL))
              cresult->addContact(Contact(tree1, tree2, root1 - tree1->getRoot(), root2));
          }
          else
          {
            Vec3f contact;
            FCL_REAL depth;
            Vec3f normal;

            if(solver->shapeTriangleIntersect(box, box_tf, p1, p2, p3, tf2, &contact, &depth, &normal))
              cresult->addContact(Contact(tree1, tree2, root1 - tree1->getRoot(), root2, contact, normal, depth));
          }

          return ((cresult->numContacts() >= crequest->num_max_contacts) && !crequest->exhaustive);
        }
        else
          return false;
      }
      else
        return false;
    }


    OBB obb1, obb2;
    convertBV(bv1, tf1, obb1);
    convertBV(tree2->getBV(root2).bv, tf2, obb2);
    if(!tree1->isNodeOccupied(root1) || !obb1.overlap(obb2)) return false;

    if(tree2->getBV(root2).isLeaf() || (root1->hasChildren() && (bv1.size() > tree2->getBV(root2).bv.size())))
    {
      for(unsigned int i = 0; i < 8; ++i)
      {
        if(root1->childExists(i))
        {
          const OcTree::OcTreeNode* child = root1->getChild(i);
          AABB child_bv;
          computeChildBV(bv1, i, child_bv);
          
          if(OcTreeMeshIntersectRecurse(tree1, child, child_bv, tree2, root2, tf1, tf2))
            return true;
        }
      }
    }
    else
    {
      if(OcTreeMeshIntersectRecurse(tree1, root1, bv1, tree2, tree2->getBV(root2).leftChild(), tf1, tf2))
        return true;

      if(OcTreeMeshIntersectRecurse(tree1, root1, bv1, tree2, tree2->getBV(root2).rightChild(), tf1, tf2))
        return true;      

    }

    return false;
  }

  bool OcTreeDistanceRecurse(const OcTree* tree1, const OcTree::OcTreeNode* root1, const AABB& bv1,
                             const OcTree* tree2, const OcTree::OcTreeNode* root2, const AABB& bv2,
                             const SimpleTransform& tf1, const SimpleTransform& tf2,
                             FCL_REAL& min_dist) const
  {
    if(!root1->hasChildren() && !root2->hasChildren())
    {
      if(tree1->isNodeOccupied(root1) && tree2->isNodeOccupied(root2))
      {
        Box box1, box2;
        SimpleTransform box1_tf, box2_tf;
        constructBox(bv1, tf1, box1, box1_tf);
        constructBox(bv2, tf2, box2, box2_tf);

        FCL_REAL dist;
        solver->shapeDistance(box1, box1_tf, box2, box2_tf, &dist);
        if(dist < min_dist) min_dist = dist;
        
        return (min_dist <= 0);
      }
      else
        return false;
    }

    if(!tree1->isNodeOccupied(root1) || !tree2->isNodeOccupied(root2)) return false;

    if(!root2->hasChildren() || (root1->hasChildren() && (bv1.size() > bv2.size())))
    {
      for(unsigned int i = 0; i < 8; ++i)
      {
        if(root1->childExists(i))
        {
          const OcTree::OcTreeNode* child = root1->getChild(i);
          AABB child_bv;
          computeChildBV(bv1, i, child_bv);

          FCL_REAL d;
          AABB aabb1, aabb2;
          convertBV(bv1, tf1, aabb1);
          convertBV(bv2, tf2, aabb2);
          d = aabb1.distance(aabb2);

          if(d < min_dist)
          {
          
            if(OcTreeDistanceRecurse(tree1, child, child_bv, tree2, root2, bv2, tf1, tf2, min_dist))
              return true;
          }
        }
      }
    }
    else
    {
      for(unsigned int i = 0; i < 8; ++i)
      {
        if(root2->childExists(i))
        {
          const OcTree::OcTreeNode* child = root2->getChild(i);
          AABB child_bv;
          computeChildBV(bv2, i, child_bv);

          FCL_REAL d;
          AABB aabb1, aabb2;
          convertBV(bv1, tf1, aabb1);
          convertBV(bv2, tf2, aabb2);
          d = aabb1.distance(aabb2);

          if(d < min_dist)
          {
            if(OcTreeDistanceRecurse(tree1, root1, bv1, tree2, child, child_bv, tf1, tf2, min_dist))
              return true;
          }
        }
      }
    }
    
    return false;
  }


  bool OcTreeIntersectRecurse(const OcTree* tree1, const OcTree::OcTreeNode* root1, const AABB& bv1,
                              const OcTree* tree2, const OcTree::OcTreeNode* root2, const AABB& bv2,
                              const SimpleTransform& tf1, const SimpleTransform& tf2) const
  {
    if(!root1->hasChildren() && !root2->hasChildren())
    {
      if(tree1->isNodeOccupied(root1) && tree2->isNodeOccupied(root2))
      {
        if(!crequest->enable_contact)
        {
          OBB obb1, obb2;
          convertBV(bv1, tf1, obb1);
          convertBV(bv2, tf2, obb2);
          
          if(obb1.overlap(obb2))
            cresult->addContact(Contact(tree1, tree2, root1 - tree1->getRoot(), root2 - tree2->getRoot()));
        }
        else
        {
          Box box1, box2;
          SimpleTransform box1_tf, box2_tf;
          constructBox(bv1, tf1, box1, box1_tf);
          constructBox(bv2, tf2, box2, box2_tf);

          Vec3f contact;
          FCL_REAL depth;
          Vec3f normal;
          if(solver->shapeIntersect(box1, box1_tf, box2, box2_tf, &contact, &depth, &normal))
            cresult->addContact(Contact(tree1, tree2, root1 - tree1->getRoot(), root2 - tree2->getRoot(), contact, normal, depth));
        }

        return ((cresult->numContacts() >= crequest->num_max_contacts) && !crequest->exhaustive);       
      }
      else
        return false;
    }

    if(!tree1->isNodeOccupied(root1) || !tree2->isNodeOccupied(root2)) return false;

    OBB obb1, obb2;
    convertBV(bv1, tf1, obb1);
    convertBV(bv2, tf2, obb2);
    if(!obb1.overlap(obb2)) return false;

    if(!root2->hasChildren() || (root1->hasChildren() && (bv1.size() > bv2.size())))
    {
      for(unsigned int i = 0; i < 8; ++i)
      {
        if(root1->childExists(i))
        {
          const OcTree::OcTreeNode* child = root1->getChild(i);
          AABB child_bv;
          computeChildBV(bv1, i, child_bv);
        
          if(OcTreeIntersectRecurse(tree1, child, child_bv, 
                                    tree2, root2, bv2,
                                    tf1, tf2))
            return true;
        }
      }
    }
    else
    {
      for(unsigned int i = 0; i < 8; ++i)
      {
        if(root2->childExists(i))
        {
          const OcTree::OcTreeNode* child = root2->getChild(i);
          AABB child_bv;
          computeChildBV(bv2, i, child_bv);
          
          if(OcTreeIntersectRecurse(tree1, root1, bv1,
                                    tree2, child, child_bv,
                                    tf1, tf2))
            return true;
        }
      }
    }

    return false;
  }
};





template<typename NarrowPhaseSolver>
class OcTreeCollisionTraversalNode : public CollisionTraversalNodeBase
{
public:
  OcTreeCollisionTraversalNode()
  {
    model1 = NULL;
    model2 = NULL;

    otsolver = NULL;
  }

  bool BVTesting(int, int) const
  {
    return false;
  }

  void leafTesting(int, int) const
  {
    otsolver->OcTreeIntersect(model1, model2, tf1, tf2, request, *result);
  }

  const OcTree* model1;
  const OcTree* model2;

  SimpleTransform tf1, tf2;

  const OcTreeSolver<NarrowPhaseSolver>* otsolver;
};


template<typename NarrowPhaseSolver>
class OcTreeDistanceTraversalNode : public DistanceTraversalNodeBase
{
public:
  OcTreeDistanceTraversalNode()
  {
    model1 = NULL;
    model2 = NULL;

    otsolver = NULL;
  }


  FCL_REAL BVTesting(int, int) const
  {
    return -1;
  }

  void leafTesting(int, int) const
  {
    min_distance = otsolver->OcTreeDistance(model1, model2, tf1, tf2);
  }

  const OcTree* model1;
  const OcTree* model2;
  
  mutable FCL_REAL min_distance;

  const OcTreeSolver<NarrowPhaseSolver>* otsolver;
};

template<typename S, typename NarrowPhaseSolver>
class ShapeOcTreeCollisionTraversalNode : public CollisionTraversalNodeBase
{
public:
  ShapeOcTreeCollisionTraversalNode()
  {
    model1 = NULL;
    model2 = NULL;

    otsolver = NULL;
  }

  bool BVTesting(int, int) const
  {
    return false;
  }

  void leafTesting(int, int) const
  {
    otsolver->OcTreeShapeIntersect(model2, *model1, tf2, tf1, request, *result);
  }

  const S* model1;
  const OcTree* model2;

  SimpleTransform tf1, tf2;

  const OcTreeSolver<NarrowPhaseSolver>* otsolver;
};

template<typename S, typename NarrowPhaseSolver>
class OcTreeShapeCollisionTraversalNode : public CollisionTraversalNodeBase
{
public:
  OcTreeShapeCollisionTraversalNode()
  {
    model1 = NULL;
    model2 = NULL;

    otsolver = NULL;
  }

  bool BVTesting(int, int) const
  {
    return false;
  }

  void leafTesting(int, int) const
  {
    otsolver->OcTreeShapeIntersect(model1, *model2, tf1, tf2, request, *result);
  }

  const OcTree* model1;
  const S* model2;

  SimpleTransform tf1, tf2;
 
  const OcTreeSolver<NarrowPhaseSolver>* otsolver;  
};

template<typename S, typename NarrowPhaseSolver>
class ShapeOcTreeDistanceTraversalNode : public DistanceTraversalNodeBase
{
public:
  ShapeOcTreeDistanceTraversalNode()
  {
    model1 = NULL;
    model2 = NULL;
    
    otsolver = NULL;
  }

  FCL_REAL BVTesting(int, int) const
  {
    return -1;
  }

  void leafTesting(int, int) const
  {
    min_distance = otsolver->OcTreeShapeDistance(model2, *model1, tf2, tf1);
  }

  const S* model1;
  const OcTree* model2;
  
  mutable FCL_REAL min_distance;

  const OcTreeSolver<NarrowPhaseSolver>* otsolver;
};

template<typename S, typename NarrowPhaseSolver>
class OcTreeShapeDistanceTraversalNode : public DistanceTraversalNodeBase
{
public:
  OcTreeShapeDistanceTraversalNode()
  {
    model1 = NULL;
    model2 = NULL;
    
    otsolver = NULL;
  }

  FCL_REAL BVTesting(int, int) const
  {
    return -1;
  }

  void leafTesting(int, int) const
  {
    min_distance = otsolver->OcTreeShapeDistance(model1, *model2, tf1, tf2);
  }

  const OcTree* model1;
  const S* model2;
  
  mutable FCL_REAL min_distance;

  const OcTreeSolver<NarrowPhaseSolver>* otsolver;
};


template<typename BV, typename NarrowPhaseSolver>
class MeshOcTreeCollisionTraversalNode : public CollisionTraversalNodeBase
{
public:
  MeshOcTreeCollisionTraversalNode()
  {
    model1 = NULL;
    model2 = NULL;

    otsolver = NULL;
  }

  bool BVTesting(int, int) const
  {
    return false;
  }

  void leafTesting(int, int) const
  {
    otsolver->OcTreeMeshIntersect(model2, model1, tf2, tf1, request, *result);
  }

  const BVHModel<BV>* model1;
  const OcTree* model2;

  SimpleTransform tf1, tf2;
    
  const OcTreeSolver<NarrowPhaseSolver>* otsolver;
};

template<typename BV, typename NarrowPhaseSolver>
class OcTreeMeshCollisionTraversalNode : public CollisionTraversalNodeBase
{
public:
  OcTreeMeshCollisionTraversalNode()
  {
    model1 = NULL;
    model2 = NULL;

    otsolver = NULL;
  }

  bool BVTesting(int, int) const
  {
    return false;
  }

  void leafTesting(int, int) const
  {
    otsolver->OcTreeMeshIntersect(model1, model2, tf1, tf2, request, *result);
  }

  const OcTree* model1;
  const BVHModel<BV>* model2;

  SimpleTransform tf1, tf2;
    
  const OcTreeSolver<NarrowPhaseSolver>* otsolver;
};


template<typename BV, typename NarrowPhaseSolver>
class MeshOcTreeDistanceTraversalNode : public DistanceTraversalNodeBase
{
public:
  MeshOcTreeDistanceTraversalNode()
  {
    model1 = NULL;
    model2 = NULL;
    
    otsolver = NULL;
  }

  FCL_REAL BVTesting(int, int) const
  {
    return -1;
  }

  void leafTesting(int, int) const
  {
    min_distance = otsolver->OcTreeMeshDistance(model2, model1, tf2, tf1);
  }

  const BVHModel<BV>* model1;
  const OcTree* model2;
  
  mutable FCL_REAL min_distance;

  const OcTreeSolver<NarrowPhaseSolver>* otsolver;

};

template<typename BV, typename NarrowPhaseSolver>
class OcTreeMeshDistanceTraversalNode : public DistanceTraversalNodeBase
{
public:
  OcTreeMeshDistanceTraversalNode()
  {
    model1 = NULL;
    model2 = NULL;
    
    otsolver = NULL;
  }

  FCL_REAL BVTesting(int, int) const
  {
    return -1;
  }

  void leafTesting(int, int) const
  {
    min_distance = otsolver->OcTreeMeshDistance(model1, model2, tf1, tf2);
  }

  const OcTree* model1;
  const BVHModel<BV>* model2;
  
  mutable FCL_REAL min_distance;

  const OcTreeSolver<NarrowPhaseSolver>* otsolver;

};



}

#endif
