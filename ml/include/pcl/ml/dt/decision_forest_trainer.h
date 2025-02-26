/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2010-2011, Willow Garage, Inc.
 *
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
 *
 */

#pragma once

#include <pcl/common/common.h>

#include <pcl/ml/dt/decision_forest.h>
#include <pcl/ml/dt/decision_tree.h>
#include <pcl/ml/dt/decision_tree_trainer.h>
#include <pcl/ml/feature_handler.h>
#include <pcl/ml/stats_estimator.h>

#include <vector>

namespace pcl {

/** Trainer for decision trees. */
template <class FeatureType,
          class DataSet,
          class LabelType,
          class ExampleIndex,
          class NodeType>
class PCL_EXPORTS DecisionForestTrainer {

public:
  /** Constructor. */
  DecisionForestTrainer();

  /** Destructor. */
  virtual ~DecisionForestTrainer();

  /** Sets the number of trees to train.
   *
   * \param[in] num_of_trees the number of trees
   */
  inline void
  setNumberOfTreesToTrain(const size_t num_of_trees)
  {
    num_of_trees_to_train_ = num_of_trees;
  }

  /** Sets the feature handler used to create and evaluate features.
   *
   * \param[in] feature_handler the feature handler
   */
  inline void
  setFeatureHandler(
      pcl::FeatureHandler<FeatureType, DataSet, ExampleIndex>& feature_handler)
  {
    decision_tree_trainer_.setFeatureHandler(feature_handler);
  }

  /** Sets the object for estimating the statistics for tree nodes.
   *
   * \param[in] stats_estimator the statistics estimator
   */
  inline void
  setStatsEstimator(
      pcl::StatsEstimator<LabelType, NodeType, DataSet, ExampleIndex>& stats_estimator)
  {
    decision_tree_trainer_.setStatsEstimator(stats_estimator);
  }

  /** Sets the maximum depth of the learned tree.
   *
   * \param[in] max_tree_depth maximum depth of the learned tree
   */
  inline void
  setMaxTreeDepth(const size_t max_tree_depth)
  {
    decision_tree_trainer_.setMaxTreeDepth(max_tree_depth);
  }

  /** Sets the number of features used to find optimal decision features.
   *
   * \param[in] num_of_features the number of features
   */
  inline void
  setNumOfFeatures(const size_t num_of_features)
  {
    decision_tree_trainer_.setNumOfFeatures(num_of_features);
  }

  /** Sets the number of thresholds tested for finding the optimal decision threshold on
   *  the feature responses.
   *
   * \param[in] num_of_threshold the number of thresholds
   */
  inline void
  setNumOfThresholds(const size_t num_of_threshold)
  {
    decision_tree_trainer_.setNumOfThresholds(num_of_threshold);
  }

  /** Sets the input data set used for training.
   *
   * \param[in] data_set the data set used for training
   */
  inline void
  setTrainingDataSet(DataSet& data_set)
  {
    decision_tree_trainer_.setTrainingDataSet(data_set);
  }

  /** Example indices that specify the data used for training.
   *
   * \param[in] examples the examples
   */
  inline void
  setExamples(std::vector<ExampleIndex>& examples)
  {
    decision_tree_trainer_.setExamples(examples);
  }

  /** Sets the label data corresponding to the example data.
   *
   * \param[in] label_data the label data
   */
  inline void
  setLabelData(std::vector<LabelType>& label_data)
  {
    decision_tree_trainer_.setLabelData(label_data);
  }

  /** Sets the minimum number of examples to continue growing a tree.
   *
   * \param[in] n number of examples
   */
  inline void
  setMinExamplesForSplit(size_t n)
  {
    decision_tree_trainer_.setMinExamplesForSplit(n);
  }

  /** Specify the thresholds to be used when evaluating features.
   *
   * \param[in] thres the threshold values
   */
  void
  setThresholds(std::vector<float>& thres)
  {
    decision_tree_trainer_.setThresholds(thres);
  }

  /** Specify the data provider.
   *
   * \param[in] dtdp the data provider that should implement getDatasetAndLabels()
   *            function
   */
  void
  setDecisionTreeDataProvider(
      typename pcl::DecisionTreeTrainerDataProvider<FeatureType,
                                                    DataSet,
                                                    LabelType,
                                                    ExampleIndex,
                                                    NodeType>::Ptr& dtdp)
  {
    decision_tree_trainer_.setDecisionTreeDataProvider(dtdp);
  }

  /** Specify if the features are randomly generated at each split node.
   *
   * \param[in] b do it or not
   */
  void
  setRandomFeaturesAtSplitNode(bool b)
  {
    decision_tree_trainer_.setRandomFeaturesAtSplitNode(b);
  }

  /** Trains a decision forest using the set training data and settings.
   *
   * \param[out] forest destination for the trained forest
   */
  void
  train(DecisionForest<NodeType>& forest);

private:
  /** The number of trees to train. */
  size_t num_of_trees_to_train_;

  /** The trainer for the decision trees of the forest. */
  pcl::DecisionTreeTrainer<FeatureType, DataSet, LabelType, ExampleIndex, NodeType>
      decision_tree_trainer_;
};

} // namespace pcl

#include <pcl/ml/impl/dt/decision_forest_trainer.hpp>
