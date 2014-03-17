/*
	Nik Zaborovsky, Feb-8
	2014
*/
#include <cstdio>
#include <vector>
#include <map>
#include <string>

#include <fstream>
#include <sstream>

#include <time.h>

#include "obj.h"
#include "counter.h"
#include "datatypes.h"
#include "definitions.h"

#ifndef _WIN32
#include <likwid.h>
#endif


#define NATIVE_ADJLIST

unsigned dataset = 1000000;
unsigned dataload = 1;
unsigned hashing = 50000000;


// Clusterize Functions
// The idea of them is to return the same number for items that supposed to be accessed together.
cluster_id cluster1(dataitem_t *i) {
	edge_t *e;
	vertex_t *v;

	if (!!(e = i->get<edge_t>())) {
		return e->src;
	}

	if (!!(v = i->get<vertex_t>())) {
		return v->id;
	}

	assert(false && "wrong data item is passed to cluster1 function" && typeid(i).name());
	return -1;
}

cluster_id cluster2(dataitem_t *i) {
	edge_t *e;
	vertex_t *v;

	if (!!(e = i->get<edge_t>())) {
		return e->dst;
	}

	if (!!(v = i->get<vertex_t>())) {
		return v->id;
	}

	assert(false && "wrong data item is passed to cluster2 function" && typeid(i).name());
	return -1;
}


const f_clusterize f2[] = {cluster2, cluster1};
typedef fmem<sizeof(f2)/sizeof(f2[0])> layout_t;
layout_t l(f2);



void load_data() {

	std::ifstream fstream;
	fstream.open("twitter_combined.txt");

	if (!fstream.is_open()) {
		assert(false && "No input file found...");
		return;
	}
	fstream.seekg (0, fstream.end);
	std::streamoff end = fstream.tellg();
	fstream.seekg (0, fstream.beg);

	std::string line;
	unsigned nul;	
	int percent = 0;

	std::map<unsigned, bool> v;
	unsigned counter = 0;
	unsigned v_num = 0, e_num = 0;
	while (std::getline(fstream, line) && counter++ < dataset) {
		if (line.empty()) continue;

		std::stringstream ss(line);
		edge_t e;
		ss >> nul;
		e.src = nul;
		ss >> nul;
		e.dst = nul;
		e.val = rand() % 40;
		l.put(e);
		e_num++;
		if (v[e.src] == 0) {
			vertex_t v1(e.src, INFINITE);
			l.put(v1);
			v[e.src] = 1;
			v_num++;
		}
		if (v[e.dst] == 0) {
			vertex_t v1(e.dst, INFINITE);
			l.put(v1);
			v[e.dst] = 1;
			v_num++;
		}
		std::streamoff length = fstream.tellg();
		if (percent < int(100 * ((double)length) / end)) {
			percent = int(100 * ((double)length) / end);
			printf("Bucketing: %d %%\r", percent);
		}
	}
	printf("Data loaded: edges=%u, vertices=%u\n", e_num, v_num);
	fstream.close();
}


void load_data_adjlist(adjlist2 &al2) {
	std::ifstream fstream;
	fstream.open("twitter_combined.txt");

	if (!fstream.is_open()) {
		assert(false && "Input file not found...");
		return;
	}
	fstream.seekg (0, fstream.end);
	std::streamoff end = fstream.tellg();
	fstream.seekg (0, fstream.beg);

	std::string line;
	unsigned nul;	
	int percent = 0;

	std::map<unsigned, bool> v;
	unsigned v_num = 0, e_num = 0;

	unsigned counter = 0;
	while (std::getline(fstream, line) && counter++ < dataset) {
		if (line.empty()) continue;

		std::stringstream ss(line);
		edge_t e;
		ss >> nul;
		e.src = nul;
		ss >> nul;
		e.dst = nul;
		e.val = rand() % 40;
		al2.push(new edge_t(e));
		e_num++;
		if (v[e.src] == 0) {
			vertex_t v1(e.src, INFINITE);
			al2.push(new vertex_t(v1));
			v[e.src] = 1;
			v_num++;
		}
		if (v[e.dst] == 0) {
			vertex_t v1(e.dst, INFINITE);
			al2.push(new vertex_t(v1));
			v[e.dst] = 1;
			v_num++;
		}

		std::streamoff length = fstream.tellg();
		if (percent < int(10000 * ((double)length) / end)) {
			percent = int(10000 * ((double)length) / end);
			printf("Bucketing: %d %%\r", percent);
		}
	}
	printf("Data loaded: edges=%u, vertices=%u\n", e_num, v_num);

	fstream.close();
}

int main(int argc, char *argv[]) {
#ifndef _WIN32
	likwid_markerInit();
#endif
	printf("Buckets3 running PageRank on 44Mb file\nFeb-Mar, 2014\nNik Zaborovsky\n");
	printf("!@#: %d %s %s\n", argc, argv[0], argv[1], argv[2]);

	if (argc >= 2) {
		dataset = atoi(argv[1]);
	}
	if (argc >= 3) {
		dataload = atoi(argv[2]);
	}
	if (argc >= 4) {
		hashing = atoi(argv[3]);
	}
#ifdef NATIVE_ADJLIST
	printf("*************** BASELINE *****************\n");
#else
	printf("*************** FRAMEWORK + ADJLIST ******\n");
#endif
	printf("DataSet size:%d\n", dataset);
	printf("Iteration:%d\n", dataload);

#ifndef NATIVE_ADJLIST
	printf("Hashing coefficient:%d\n", hashing);
#endif
	
	adjlist2 al2;

#ifdef NATIVE_ADJLIST
	load_data_adjlist(al2);
#else
	load_data();
	l.make_layout_3();
	l.load_file();
#endif

#ifndef NATIVE_ADJLIST
	{
#ifdef _PRINT_LAYOUT
		printf("HOW DATA IS PUT TO ADJLIST (ORDER)\n");
#endif
		std::map<unsigned, vertex_t*> vertex_by_id;
		for (layout_t::typed_iterator<vertex_t> it =
				 l.query_type<vertex_t>(0);
				 !it.end(); ++it) {
			vertex_t *v = const_cast<vertex_t*>(it.operator()());
			al2.push(v);
		}


		for (layout_t::typed_iterator<edge_t> it =
				 l.query_type<edge_t>(0);
				 !it.end(); ++it) {
			edge_t *e = const_cast<edge_t*>(it.operator()());
#ifdef _PRINT_LAYOUT
			e->print();
			printf("\n");
#endif
			al2.push(e);
		}

	}

	l.clearMaps();
#endif

	al2.create_lists();

	unsigned in_ed = 0;
	unsigned out_ed = 0;

	const unsigned vcount = al2.vertices.size();
	printf("vcount=%u\n", vcount);

	std::multimap<unsigned, vertex_t*> Q_orig;
	std::map<unsigned, vertex_t*> vertex_by_id;

	for (vertex_t* v : al2.vertices) {
		vertex_by_id[v->id] = v;
		Q_orig.insert(std::make_pair(v->val, v));
	}

	std::multimap<unsigned, vertex_t*> Q;
	int loop;
	std::multimap<unsigned, vertex_t*>::iterator source_it;

	time_t t1 = time(0);

#ifndef _WIN32
	likwid_markerStartRegion("Execution");
#endif

	for (vertex_t* v : al2.vertices) {
		v->val = INFINITE;
	}

	Q = Q_orig;
	source_it = Q.begin();
	int n = loop;
	while (n--) { ++source_it; };

	vertex_t *source = source_it->second;
	source->val = 0;

	Q.erase(source_it);
	Q.insert(std::make_pair(source->val, source));

	for (loop = 0; loop < dataload; ++loop) {

		while(!Q.empty()) {
			vertex_t *u = Q.begin()->second;
			Q.erase(Q.begin());
			if (INFINITE == u->val)
				break;
			for (edge_t *e : al2.out_edges[u]) {
				vertex_t *v = vertex_by_id[e->dst];
				unsigned distance = u->val + e->val;
				if (distance < v->val) {
					std::multimap<unsigned, vertex_t*>::iterator it = Q.find(v->val);
					while (it != Q.end() && it->first == v->val && it->second != v) { ++it; };
					assert(it != Q.end() && "Vertex not found... :(");
					Q.erase(it);
					v->val = distance;
					Q.insert(std::make_pair(v->val, v));
				}
			}
			for (edge_t *e : al2.in_edges[u]) {
				vertex_t *v = vertex_by_id[e->src];
				unsigned distance = u->val + e->val;
				if (distance < v->val) {
					std::multimap<unsigned, vertex_t*>::iterator it = Q.find(v->val);
					while (it != Q.end() && it->first == v->val && it->second != v) { ++it; };
					assert(it != Q.end() && "Vertex not found... :(");
					Q.erase(it);
					v->val = distance;
					Q.insert(std::make_pair(v->val, v));
				}
			}
		}
	}
//	for (unsigned i = 0; i < vcount; ++i) {
//		
//#ifdef _PRINT_LAYOUT
//		printf("-------------------------------------------------------------\n");
//		al2.vertices[i]->print();
//		scounter.ref((size_t)al2.vertices[i]);
//#endif
//#ifdef _PRINT_LAYOUT
//		printf("\n");
//#endif
//		std::vector<edge_t*> &in_edges = al2.in_edges[al2.vertices[i]];
//		int sum = 0;
//
//		for (unsigned j = 0; j < in_edges.size(); ++j) {
//#ifdef _PRINT_LAYOUT
//			in_edges[j]->print();
//			scounter.ref((size_t)in_edges[j]);
//#endif
//			sum += in_edges[j]->val;
//#ifdef _PRINT_LAYOUT
//			printf("\n");
//#endif
//		}
//#ifdef _PRINT_LAYOUT
//		printf("--><---\n");
//#endif
//		std::vector<edge_t*> &out_edges = al2.out_edges[al2.vertices[i]];
//
//		int count = out_edges.size();
//		for (unsigned j = 0; j < out_edges.size(); ++j) {
//
//#ifdef _PRINT_LAYOUT
//			out_edges[j]->print();
//			scounter.ref((size_t)out_edges[j]);
//#endif
//			out_edges[j]->val = sum / count;
//#ifdef _PRINT_LAYOUT
//			printf("\n");
//#endif
//		}
//	}
//#ifndef _WIN32
//	likwid_markerStopRegion("Execution");	
//#endif

	time_t t2 = time(0);

	printf("Elapsed time: %lf\n", difftime(t2, t1));
	//scounter.print_summary();
#ifndef _WIN32
	likwid_markerClose();
#endif
#ifdef _WIN32
	getchar();
#endif
	return 1;
}
