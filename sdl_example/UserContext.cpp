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

void UserContext::MergeClusters(const int& clu1, const int& clu2) {
	last_user_action_ = UA_MERGE_CLUSTERS;
	last_user_action_id_ ++;
}

void UserContext::CutSpikes(const int& clu) {
	last_user_action_ = UA_CUT_SPIKES;
	last_user_action_id_ ++;
}

void UserContext::CreateClsuter(const int& clu) {
	last_user_action_ = UA_CREATE_CLUSTER;
	last_user_action_id_ ++;
}

void UserContext::DelleteCluster(const int& clu) {
	last_user_action_ = UA_DELETE_CLUSTER;
	last_user_action_id_ ++;
}

bool UserContext::HasNewAction(const unsigned int& ref_ua_id) {
	return last_user_action_id_ > ref_ua_id;
}

bool UserContext::IsSelected(Spike* spike) {
	return spike->cluster_id_ > 0 && (spike->cluster_id_ == selected_cluster1_ || spike->cluster_id_ == selected_cluster2_);
}

UserContext::~UserContext() {
	// TODO Auto-generated destructor stub
}

