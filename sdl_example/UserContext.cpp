/*
 * UserContext.cpp
 *
 *  Created on: Oct 6, 2014
 *      Author: igor
 */

#include "UserContext.h"

UserContext::UserContext() {
	// TODO Auto-generated constructor stub
}

void UserContext::SelectCluster1(const int& clu) {
	selected_cluster1_ = clu;
	last_user_action_ = UA_SELECT_CLUSTER1;
	last_user_action_id_ ++;
}

void UserContext::SelectCluster2(const int& clu) {
	selected_cluster2_ = clu;
	last_user_action_ = UA_SELECT_CLUSTER2;
	last_user_action_id_ ++;
}

void UserContext::MergeClusters(PolygonCluster clu1, PolygonCluster clu2) {
	last_user_action_ = UA_MERGE_CLUSTERS;
	last_user_action_id_ ++;

	action_list_.push_back(UserAction(UA_MERGE_CLUSTERS, selected_cluster1_, selected_cluster2_, clu1, clu2));
	invalid_cluster_numbers_[active_tetrode_].push_back(selected_cluster2_);

	selected_cluster2_ = -1;
}

void UserContext::CutSpikes(const int& clu) {
	last_user_action_ = UA_CUT_SPIKES;
	last_user_action_id_ ++;
}

int UserContext::CreateClsuter(const int& maxclu, PolygonClusterProjection proj) {
	last_user_action_ = UA_CREATE_CLUSTER;
	last_user_action_id_ ++;

	int clun = maxclu;

	if (!invalid_cluster_numbers_[active_tetrode_].empty()){
		clun = invalid_cluster_numbers_[active_tetrode_].front();
		invalid_cluster_numbers_[active_tetrode_].pop_front();
	}

	action_list_.push_back(UserAction(UA_CREATE_CLUSTER, clun, proj));

	return clun;
}

void UserContext::DelleteCluster(PolygonCluster& cluster) {
	last_user_action_ = UA_DELETE_CLUSTER;
	last_user_action_id_ ++;

	cluster.Invalidate();
	invalid_cluster_numbers_[active_tetrode_].push_back(selected_cluster2_);

	action_list_.push_back(UserAction(UA_DELETE_CLUSTER, selected_cluster2_, cluster));
	selected_cluster2_ = -1;
}

bool UserContext::HasNewAction(const unsigned int& ref_ua_id) {
	return last_user_action_id_ > ref_ua_id;
}

bool UserContext::IsSelected(Spike* spike) {
	return spike->cluster_id_ > 0 && (spike->cluster_id_ == selected_cluster1_ || spike->cluster_id_ == selected_cluster2_);
}

void UserContext::AddExclusiveProjection(PolygonClusterProjection proj) {
	action_list_.push_back(UserAction(UA_ADD_EXCLUSIVE_PROJECTION, selected_cluster2_, proj));
}

void UserContext::AddInclusiveProjection(PolygonClusterProjection proj) {
	action_list_.push_back(UserAction(UA_ADD_INCLUSIVE_PROJECTION, selected_cluster2_, proj));
}

UserContext::~UserContext() {
	// TODO Auto-generated destructor stub
}

UserAction::UserAction(UserActionType action_type, int cluster_number,
		PolygonCluster poly_clust)
: action_type_(action_type)
, cluster_number_1_(cluster_number)
, poly_clust_1_(poly_clust)
{
}

UserAction::UserAction(UserActionType action_type, int cluster_number_1,
		int cluster_number2, PolygonCluster poly_clust_1,
		PolygonCluster poly_clust_2)
: action_type_(action_type)
, cluster_number_1_(cluster_number_1)
, poly_clust_1_(poly_clust_1)
, cluster_number_2_(cluster_number2)
, poly_clust_2_(poly_clust_2)
{
}

UserAction::UserAction(UserActionType action_type, int cluster_number,
		PolygonClusterProjection projection)
: action_type_(action_type)
, cluster_number_1_(cluster_number)
, projection_(projection)
{
}

void UserContext::Init(int tetrodes_number) {
	invalid_cluster_numbers_.resize(tetrodes_number);
}
