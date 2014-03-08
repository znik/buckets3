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

#ifndef _WIN32
#include <likwid.h>
#endif


unsigned dataset = 100000;
unsigned dataload = 1;
unsigned hashing = 1;


struct edge_t : public dataitem_t {
	int src;
	int dst;
	int val;
	edge_t(int a = 0, int b = 0, int v = 0) : src(a), dst(b), val(v) {};
	void print() {
		printf("E: %d->%d [0x%p]\n", src, dst, this);
	}
};

struct vertex_t : public dataitem_t {
	int id;
	int val;
	vertex_t(int a = 0, int b = 0) : id(a), val(b) {};
	void print() {
		printf("V: %d [0x%p]\n", id, this);
	}
};

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


const f_clusterize f2[] = {cluster1, cluster2};
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

	std::vector<unsigned> v;
	
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
		l.put(e);
		if (v.end() == std::find(v.begin(), v.end(), e.src)) {
			vertex_t v1(e.src, rand() % 40);
			v.push_back(v1.id);
			l.put(v1);
		}
		if (v.end() == std::find(v.begin(), v.end(), e.dst)) {
			vertex_t v1(e.dst, rand() % 40);
			v.push_back(v1.id);
			l.put(v1);
		}
		std::streamoff length = fstream.tellg();
		if (percent < int(100 * ((double)length) / end)) {
			percent = int(100 * ((double)length) / end);
			printf("Bucketing: %d %%\r", percent);
		}
	}
	fstream.close();
}


int main(int argc, char *argv[]) {
#ifndef _WIN32
	likwid_markerInit();
#endif
	printf("Buckets3 running PageRank on 44Mb file\nFeb-Mar, 2014\nNik Zaborovsky\n");
	printf("!@#: %d %s %s\n", argc, argv[0], argv[1]);

	if (argc >= 2) {
		dataset = atoi(argv[1]);
	}
	if (argc >= 3) {
		dataload = atoi(argv[2]);
	}
	if (argc >= 4) {
		hashing = atoi(argv[3]);
	}
	printf("DataSet size:%d\n", dataset);
	printf("Iteration:%d\n", dataload);
	printf("Hashing coefficient:%d\n", hashing);

//	load_data();
//	l.make_layout_3();
	l.load_file();


	time_t t1 = time(0);
	unsigned in_ed = 0;

	unsigned out_ed = 0;
#ifndef _WIN32
	likwid_markerStartRegion("Execution");
#endif

	layout_t::cluster_iterator<edge_t> it2(0);
	layout_t::cluster_iterator<edge_t> it3(1);

	for (unsigned loop = 0; loop < 100; ++loop)
	{

	for (layout_t::typed_iterator<vertex_t> it = l.query_type<vertex_t>(); !it.end(); ++it) {

		vertex_t *v = const_cast<vertex_t*>(it.operator()());

		l.go_to_cluster(it2, v);
		l.go_to_cluster(it3, v);

		int sum = 0;
		in_ed += it2.size();

		for (; !it2.end(); ++it2) {
			edge_t *e = it2.operator()();
			sum += e->val;
		}
	
		out_ed += it3.size();
		int count = it3.size();
		for (; !it3.end(); ++it3) {
			edge_t *e = it3.operator()();
			e->val = sum / count;
		}
	}
	}
#ifndef _WIN32
	likwid_markerStopRegion("Execution");	
#endif	
	time_t t2 = time(0);
	printf("Graph summary:\nin_edges=%u\nout_edges=%u\n", in_ed, out_ed);

	printf("Elapsed time: %lf\n", difftime(t2, t1));

#ifndef _WIN32
#endif
#ifndef _WIN32
	likwid_markerClose();
#endif

	return 1;
}
