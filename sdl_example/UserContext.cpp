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

void UserContext::SelectCluster1(const int& clu, const unsigned int& pkg_id) {
	selected_cluster1_ = clu;
	last_user_action_time_ = pkg_id;
	last_user_action_ = UA_SELECT_CLUSTER1;
}

void UserContext::SelectCluster2(const int& clu, const unsigned int& pkg_id) {
	selected_cluster2_ = clu;
	last_user_action_time_ = pkg_id;
	last_user_action_ = UA_SELECT_CLUSTER2;
}

void UserContext::MergeClusters(const int& clu1, const int& clu2,
		const unsigned int& pkg_id) {
	last_user_action_time_ = pkg_id;
	last_user_action_ = UA_MERGE_CLUSTERS;
}

void UserContext::CutSpikes(const int& clu, const unsigned int& pkg_id) {
	last_user_action_time_ = pkg_id;
	last_user_action_ = UA_CUT_SPIKES;
}

void UserContext::CreateClsuter(const int& clu, const unsigned int& pkg_id) {
	last_user_action_time_ = pkg_id;
	last_user_action_ = UA_CREATE_CLUSTER;
}

void UserContext::DelleteCluster(const int& clu, const unsigned int& pkg_id) {
	last_user_action_time_ = pkg_id;
	last_user_action_ = UA_DELETE_CLUSTER;
}

bool UserContext::HasNewAction(const unsigned int& ref_pkg_id) {
	return last_user_action_time_ > ref_pkg_id;
}

UserContext::~UserContext() {
	// TODO Auto-generated destructor stub
}

