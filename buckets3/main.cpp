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
#include "datatypes.h"
#include "definitions.h"

#ifndef _WIN32
#include <likwid.h>
#endif


//#define NATIVE_ADJLIST


unsigned dataset = 10000;
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

cluster_id cluster3(dataitem_t *i) {
	edge_t *e;
	vertex_t *v;

	if (!!(e = i->get<edge_t>())) {
		return e->src;
	}

	if (!!(v = i->get<vertex_t>())) {
		return v->id;
	}

	assert(false && "wrong data item is passed to cluster2 function" && typeid(i).name());
	return -1;
}


const f_clusterize f2[] = {cluster1, cluster2};
typedef fmem<sizeof(f2)/sizeof(f2[0])> layout_t;
layout_t l(f2);

//const f_clusterize f2[] = {cluster3};
//typedef fmem<sizeof(f2)/sizeof(f2[0])> layout_t;
//layout_t l(f2);


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
			vertex_t v1(e.src, rand() % 40);
			l.put(v1);
			v[e.src] = 1;
			v_num++;
		}
		if (v[e.dst] == 0) {
			vertex_t v1(e.dst, rand() % 40);
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
			vertex_t v1(e.src, rand() % 40);
			al2.push(new vertex_t(v1));
			v[e.src] = 1;
			v_num++;
		}
		if (v[e.dst] == 0) {
			vertex_t v1(e.dst, rand() % 40);
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
		for (layout_t::typed_iterator<edge_t> it =
				 l.query_type<edge_t>(0);
				 !it.end(); ++it) {
			edge_t *e = const_cast<edge_t*>(it.operator()());
			al2.push(e);
		}

		for (layout_t::typed_iterator<vertex_t> it =
			 l.query_type<vertex_t>(0);
			 !it.end(); ++it) {
			
			vertex_t *v = const_cast<vertex_t*>(it.operator()());
			al2.push(v);
		}		
	}

	l.clearMaps();
#endif

	al2.create_lists();

	unsigned in_ed = 0;
	unsigned out_ed = 0;

	unsigned vcount = al2.vertices.size();
	printf("vcount=%u\n", vcount);

	time_t t1 = time(0);

	void *prev = al2.vertices[0];
	int irregularity = 0;

#ifndef _WIN32
	likwid_markerStartRegion("Execution");
#endif
	for (unsigned loop = 0; loop < dataload; ++loop)
	for (unsigned i = 0; i < vcount; ++i) {


		std::vector<edge_t*> &in_edges = al2.in_edges[al2.vertices[i]];
		//printf("v=%X\n", al2->vertices[i]);	

		//al2.vertices[i]->print();
		if (prev > (void*)al2.vertices[i]) {
			irregularity++;
			//printf(" <<<<< ");
		}
		prev = al2.vertices[i];
		//printf("\n");
		std::vector<edge_t*> &out_edges = al2.out_edges[al2.vertices[i]];

		int sum = 0;

		for (unsigned j = 0; j < in_edges.size(); ++j) {
			//printf("e=%X\n", in_edges[j]);
			//in_edges[j]->print();
			sum += in_edges[j]->val;
			if (prev > (void*)in_edges[j]) {
				irregularity++;
				//printf(" <<<<< ");
			}
			prev = in_edges[j];
			//printf("\n");
		}
		int count = out_edges.size();
		for (unsigned j = 0; j < out_edges.size(); ++j) {
			//printf("e=%X\n", out_edges[j]);
			//out_edges[j]->print();
			out_edges[j]->val = sum / count;
			if (prev > (void*)out_edges[j]) {
				irregularity++;
				//printf(" <<<<< ");
			}
			prev = out_edges[j];
			//printf("\n");
		}
	}
#ifndef _WIN32
	likwid_markerStopRegion("Execution");	
#endif

	time_t t2 = time(0);
//	printf("Graph summary:\nin_edges=%u\nout_edges=%u\n", in_ed, out_ed);

	printf("Elapsed time: %lf\n", difftime(t2, t1));
	printf("SUPER IRREGULARITY: %d\n", irregularity);

#ifndef _WIN32
	likwid_markerClose();
#endif
	getchar();
	return 1;
}
