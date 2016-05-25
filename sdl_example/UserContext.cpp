/*
 * UserContext.cpp
 *
 *  Created on: Oct 6, 2014
 *      Author: igor
 */

#include "UserContext.h"

unsigned int UserAction::last_id_ = 0;

UserContext::UserContext() {
}

void UserContext::SelectCluster1(const int& clu) {
	action_list_.push_back(UserAction(UA_SELECT_CLUSTER1, clu));
	selected_cluster1_ = clu;
}

void UserContext::SelectCluster2(const int& clu) {
	action_list_.push_back(UserAction(UA_SELECT_CLUSTER2, clu));
	selected_cluster2_ = clu;
}

void UserContext::MergeClusters(PolygonCluster& clu1, PolygonCluster& clu2) {
	action_list_.push_back(UserAction(UA_MERGE_CLUSTERS, selected_cluster1_, selected_cluster2_, clu1, clu2));
	clu2.Invalidate();

	selected_cluster2_ = -1;
}

void UserContext::CutSpikes(const int& clu) {
	action_list_.push_back(UserAction(UA_CUT_SPIKES, clu));
}

int UserContext::CreateClsuter(const int& maxclu, PolygonClusterProjection proj) {
	int clun = maxclu;

	action_list_.push_back(UserAction(UA_CREATE_CLUSTER, clun, proj));

	return clun;
}

void UserContext::DelleteCluster(PolygonCluster& cluster) {
	cluster.Invalidate();

	action_list_.push_back(UserAction(UA_DELETE_CLUSTER, selected_cluster2_, cluster));
	selected_cluster2_ = -1;
}

bool UserContext::HasNewAction(const unsigned int& ref_ua_id) {
	return ! action_list_.empty() && ref_ua_id < action_list_.back().id_;
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
}

UserAction::UserAction(UserActionType action_type, int cluster_number,
		PolygonCluster poly_clust)
: action_type_(action_type)
, cluster_number_1_(cluster_number)
, poly_clust_1_(poly_clust)
, id_(++ last_id_)
{
}

UserAction::UserAction(UserActionType action_type, int cluster_number_1,
		int cluster_number2, PolygonCluster poly_clust_1,
		PolygonCluster poly_clust_2)
: action_type_(action_type)
, cluster_number_1_(cluster_number_1)
, cluster_number_2_(cluster_number2)
, poly_clust_1_(poly_clust_1)
, poly_clust_2_(poly_clust_2)
, id_(++ last_id_)
{
}

UserAction::UserAction(UserActionType action_type, int cluster_number)
: action_type_(action_type)
, cluster_number_1_(cluster_number)
, id_(++ last_id_ )
{
}

UserAction::UserAction(UserActionType action_type, int cluster_number,
		PolygonClusterProjection projection)
: action_type_(action_type)
, cluster_number_1_(cluster_number)
, projection_(projection)
, id_(++ last_id_)
{
}

const UserAction* UserContext::GetNextAction(
		const unsigned int& ref_action_id) {
	for (std::list<UserAction>::const_reverse_iterator ua_iter = action_list_.rbegin();  ua_iter != action_list_.rend(); ++ua_iter){
		if (ua_iter->id_ <= ref_action_id && ua_iter != action_list_.rbegin()){
			return &(*(--ua_iter ));
		}
	}

	// if first action is new
	if (action_list_.back().id_ > ref_action_id){
		return &(action_list_.back());
	}
	else{
		return nullptr;
	}
}
