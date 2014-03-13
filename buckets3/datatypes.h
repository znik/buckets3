#pragma once

#include "obj.h"


struct edge_t : public dataitem_t {
	int src;
	int dst;
	int val;
	edge_t(int a = 0, int b = 0, int v = 0) : src(a), dst(b), val(v) {};
	void print() {
		printf("E: %15d->%15d [0x%p]", src, dst, this);
	}
};

struct vertex_t : public dataitem_t {
	int id;
	int val;
	vertex_t(int a = 0, int b = 0) : id(a), val(b) {};
	void print() {
		printf("V: %15d                  [0x%p]", id, this);
	}
};
