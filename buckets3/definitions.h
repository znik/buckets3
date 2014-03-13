#pragma once

#include <queue>
#include "obj.h"


struct adjlist2 {
	// vertex -> {edges}
	std::map<vertex_t*, std::vector<edge_t*> > in_edges;
	std::map<vertex_t*, std::vector<edge_t*> > out_edges;

	std::deque<edge_t*> edges;
	std::deque<vertex_t*> vertices;

	// Initializes lists
	void push(vertex_t *src) {
		bool found = false;
		for (unsigned i = 0; i < vertices.size(); ++i) {
			vertex_t *v = vertices[i];	
			if (v->id == src->id) {
				found = true;
				break;
			}
		}

		if (!found)
			vertices.push_back(src);
	}

	void push(edge_t *e) {
		edges.push_back(e);
	}

	void create_lists() { // O(|E||V|)
		printf("SUMMARY\n|E|=%d\n|V|=%d\n", edges.size(), vertices.size());

		for (unsigned i = 0; i < edges.size(); ++i) {
			int src_idx = 0, dst_idx = 0;
			for (unsigned j = 0; j < vertices.size(); ++j) {
				if (vertices[j]->id == edges[i]->src) {
					src_idx = j;
					break;
				}
			}
			for (unsigned j = 0; j < vertices.size(); ++j) {
				if (vertices[j]->id == edges[i]->dst) {
					dst_idx = j;
					break;
				}
			}
			in_edges[vertices[dst_idx]].push_back(edges[i]);
			out_edges[vertices[src_idx]].push_back(edges[i]);
		}

		//for (std::map<vertex_t*, std::vector<edge_t*> >::iterator it = in_edges.begin(); in_edges.end() != it;
		//	++it) {
		//	std::sort(it->second.begin(), it->second.end());
		//}

		//for (std::map<vertex_t*, std::vector<edge_t*> >::iterator it = out_edges.begin(); out_edges.end() != it;
		//	++it) {
		//	std::sort(it->second.begin(), it->second.end());
		//}


		printf("|in_edges|=%d\n|out_edges|=%d\n", in_edges.size(), out_edges.size());
	}

};
