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

	void create_lists() { // O(|E| + |V| log|E|)
		printf("SUMMARY\n|E|=%d\n|V|=%d\n", edges.size(), vertices.size());
		// TODO: redo using maps and one cycle by vertex and two inside by vertices - filling the map
	
		std::map<unsigned, std::vector<edge_t*>> edge_by_src;
		std::map<unsigned, std::vector<edge_t*>> edge_by_dst;

		for (unsigned i = 0; i < edges.size(); ++i) {
			edge_by_src[edges[i]->src].push_back(edges[i]);
			edge_by_dst[edges[i]->dst].push_back(edges[i]);
		}

		for (unsigned i = 0; i < vertices.size(); ++i) {
			for (unsigned j = 0; j < edge_by_dst[vertices[i]->id].size(); ++j)
				in_edges[vertices[i]].push_back(edge_by_dst[vertices[i]->id][j]);
			for (unsigned j = 0; j < edge_by_src[vertices[i]->id].size(); ++j)
				out_edges[vertices[i]].push_back(edge_by_src[vertices[i]->id][j]);			
		}
/*
		for (unsigned i = 0; i < edges.size(); ++i) {
			int two = 2;
			for (unsigned j = 0; j < vertices.size() && two > 0; ++j) {
				if (vertices[j]->id == edges[i]->src) {
					in_edges[vertices[j]].push_back(edges[i]);
					--two;
				}
				if (vertices[j]->id == edges[i]->dst) {
					out_edges[vertices[j]].push_back(edges[i]);
					--two;
				}
			}
		}
*/
/*
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
*/

		printf("|in_edges|=%d\n|out_edges|=%d\n", in_edges.size(), out_edges.size());
	}

};
