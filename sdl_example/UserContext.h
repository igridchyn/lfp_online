/*
 * UserContext.h
 *
 *  Created on: Oct 6, 2014
 *      Author: igor
 */

#ifndef USERCONTEXT_H_
#define USERCONTEXT_H_

#include "Spike.h"
#include "PolygonCluster.h"

#include <list>

enum UserActionType{
	UA_NONE,
	UA_SELECT_CLUSTER1,
	UA_SELECT_CLUSTER2,
	UA_CREATE_CLUSTER,
	UA_MERGE_CLUSTERS,
	// add exclusive projection
	UA_CUT_SPIKES,
	UA_DELETE_CLUSTER,

	// TODO: show in which dimensions cluster has projections
	UA_ADD_INCLUSIVE_PROJECTION,
	UA_ADD_EXCLUSIVE_PROJECTION,
	UA_REMOVE_PROJECTION
};

// class with big overhead, but number of stored actions is not large
class UserAction{

	friend class UserContext;

protected:


	static unsigned int last_id_;

public:
	UserActionType action_type_;
	PolygonCluster poly_clust_1_;
	PolygonCluster poly_clust_2_;
	// for projection actions
	PolygonClusterProjection projection_;

	int cluster_number_1_ = -1;
	int cluster_number_2_ = -1;

	const unsigned int id_;

	// select / cut
	UserAction(UserActionType action_type, int cluster_number);

	// delete, add
	UserAction(UserActionType action_type, int cluster_number, PolygonCluster poly_clust);
	// merge
	UserAction(UserActionType action_type, int cluster_number_1, int cluster_number2, PolygonCluster poly_clust_1, PolygonCluster poly_clust_2);
	// for projection adding / deleting projections
	UserAction(UserActionType action_type, int cluster_number, PolygonClusterProjection projection);
};

// ALL ACTIONS ARE PERFORMED FOR ACTIVE TETRODE !
class UserContext {
public:
	unsigned int active_tetrode_ = 0;

	// using double-linked list to enable undo
	std::vector<std::list <int> > invalid_cluster_numbers_;
	// !!! ASSUMPTION: last user action is processed by all targeted processors before it can be undone
	// 		the same applies to un/re-doing : it is processed before new action can be introduced
	std::list<UserAction> action_list_;

	// TODO make private
	int selected_cluster1_ = -1;
	int selected_cluster2_ = -1;

	void SelectCluster1(const int& clu);
	void SelectCluster2(const int& clu);
	void MergeClusters(PolygonCluster clu1, PolygonCluster clu2);
	void CutSpikes(const int& clu);
	int CreateClsuter(const int& maxclu, PolygonClusterProjection proj);
	void DelleteCluster(PolygonCluster& cluster);
	void AddExclusiveProjection(PolygonClusterProjection proj);
	void AddInclusiveProjection(PolygonClusterProjection proj);

	bool HasNewAction(const unsigned int& ref_action_id);
	const UserAction* GetNextAction(const unsigned int& ref_action_id);

	bool IsSelected(Spike *spike);

	UserContext();
	virtual ~UserContext();

	void Init(int tetrodes_number);
};

#endif /* USERCONTEXT_H_ */
