/*
 * UserContext.h
 *
 *  Created on: Oct 6, 2014
 *      Author: igor
 */

#ifndef USERCONTEXT_H_
#define USERCONTEXT_H_

enum UserAction{
	UA_NONE,
	UA_SELECT_CLUSTER1,
	UA_SELECT_CLUSTER2,
	UA_CREATE_CLUSTER,
	UA_MERGE_CLUSTERS,
	// add exclusive projection
	UA_CUT_SPIKES,
	UA_DELETE_CLUSTER
};

class UserContext {
public:

	// TODO make private
	int selected_cluster1_ = -1;
	int selected_cluster2_ = -1;

	// !!! it is assumed that all processors have enough time to process user action before it is updated
	// it is also assumed that operations are performed over selected clusters and if selection changes, UA a is reset to NULL
	UserAction last_user_action_;
	// in packages (1 / <sampling rate> s)
	unsigned int last_user_action_id_;

	void SelectCluster1(const int& clu);
	void SelectCluster2(const int& clu);
	void MergeClusters(const int& clu1, const int& clu2);
	void CutSpikes(const int& clu);
	void CreateClsuter(const int& clu);
	void DelleteCluster(const int& clu);

	bool HasNewAction(const unsigned int& ref_pkg_id);

	UserContext();
	virtual ~UserContext();
};

#endif /* USERCONTEXT_H_ */
