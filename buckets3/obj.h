/*
	Nik Zaborovsky, Feb-8
	2014
*/
#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

#include <algorithm>
#include <functional>
#include <sstream>
#include <cassert>
#include <map>
#include <string>
#include <memory.h>

#include "layout.h"

#ifdef _WIN32
const TCHAR *mappedFile = TEXT("mapped.file");
#endif

extern unsigned hashing;


//#define _PRINT_LAYOUT

// Every dataitem should be derived from this interface.
struct dataitem_t {
public:
	template <typename T>
	T* get() const {
		if ((unsigned)type_hash != (unsigned)typeid(T).hash_code())
			return 0;
		//assert(0 != dynamic_cast<const T *const>(this));
		return const_cast<T*>(static_cast<const T *const>(this));
	}

	template <typename T>
	T* get_direct() const {
		return (T*)this;
	}

	virtual ~dataitem_t() {}; // * makes structure polymorhpic
	virtual void print() = 0;
	size_t	type_hash; // * will be initialized after first usage!
	size_t	type_size; // *
	// TODO: hide internals from the programmer
};

namespace {
	typedef unsigned cluster_id;
	typedef cluster_id (*f_clusterize)(dataitem_t *item);
	typedef std::vector<dataitem_t *> cluster_t;
	typedef std::map<cluster_id, cluster_t> clusters_t;
	typedef std::map<size_t, clusters_t> clusters_and_types_t;
	typedef std::vector<clusters_and_types_t> maps_t;
};

namespace {
	template<unsigned N>
	struct signature_t {
		unsigned u[N];

		void print() const {
			for (int i = N - 1; i >= 0; --i) {
				printf("%u.", u[i]);
			}
			printf("\n");
		}
	};

	template<unsigned N>
	bool operator<(const signature_t<N>& s1, const signature_t<N>& s2) {
		return 0 < memcmp(&s1, &s2, sizeof(s2));
	}
}

template <unsigned N>
class fmem {
	typedef std::map<signature_t<N>, cluster_t> virtual_buckets_t;

public:

	fmem(const f_clusterize f[]) : layout(0) {
		for (unsigned i = 0; i < N; ++i) {
			clusterize.push_back(f[i]);
		}
		m_datasize = 0;
	}

	~fmem() {
		if (0 != layout) {
#ifndef _WIN32
			delete [] layout;
			//free(layout);
#endif
		}
		// TODO: Unmap etc.
		layout = 0;
	};

public:
	template <typename T> void put(T& t) {
		compile_time_check_not_derived_from_dataitem_t<T>();

		assert(0 != dynamic_cast<dataitem_t *>(&t));	
		if (0 == static_cast<dataitem_t *>(&t))
			return;
		T *copy = new T(t);
		dataitem_t *di = static_cast<dataitem_t *>(copy);
		di->type_hash = (unsigned)typeid(T).hash_code();
		di->type_size = sizeof(T);

		_add_dataitem_5(di);
	};
/*
	void print() {
		for (typename virtual_buckets_t::const_iterator it = vbuckets.begin(); vbuckets.end() != it; ++it) {
			const cluster_t &c = it->second;
			printf("Bucket: %s\n", it->first.c_str());
			for (unsigned i = 0; i < c.size(); ++i) {
				c[i]->print();
			}
		}
	}
*/

	void make_layout_3() {
		for (typename hash_and_vbuckets_t::iterator it = vbuckets.begin(); vbuckets.end() != it; ++it) {
			printf("\nBUCKETS_NUM: %u\n", (unsigned)it->second.size());
		}

		struct _comparator {
			const std::vector<f_clusterize> &clusterize;

			_comparator(const std::vector<f_clusterize> &_clusterize) : clusterize(_clusterize) {};

			bool operator() (dataitem_t *a, dataitem_t *b) {
				//if (a->type_hash != b->type_hash)
				//	return a->type_hash > b->type_hash;

				for (int i = N - 1; i >= 0; --i) {
					if (clusterize[i](a) != clusterize[i](b))
						return clusterize[i](a) > clusterize[i](b);
				}
				return false;
			}
		} comparator(clusterize);

		for (typename hash_and_vbuckets_t::iterator it = vbuckets.begin(); vbuckets.end() != it; ++it) {
			for (typename virtual_buckets_t::iterator it2 = it->second.begin(); it->second.end() != it2; ++it2) {
				std::sort(it2->second.begin(), it2->second.end(), comparator);
			}
		}




#ifdef _PRINT_LAYOUT
			printf("LAYOUT:\n\n");
			for (typename hash_and_vbuckets_t::iterator it = vbuckets.begin(); vbuckets.end() != it; ++it) {
				for (typename virtual_buckets_t::reverse_iterator it2 = it->second.rbegin(); it->second.rend() != it2; ++it2) {
					printf("---- BUCKET: ");
					const signature_t<N> &s = it2->first;
					s.print();
					const cluster_t &c = it2->second;
					for (int i = c.size() - 1; i >= 0; --i) {
						c[i]->print();
						printf("\n");
					}
				}
			}
#endif

		size_t datatotal = 0;

		// maps - items ordered by types in every bucket
		// maps[i] - items according to one distribution i
		// maps[i][cluster_id] - all items from bucket i that correspond to cluster_id
		// maps[i][cluster_id][hash] - all items from bucket i that correspond to cluster_id of type hash
		typedef std::vector<item_offset_t> offsets_t;
		std::vector<std::map<cluster_id, std::map<size_t, offsets_t> > > maps(N);

		// types
		// hash(T) - sizeof(T) pairs
		std::map<size_t, size_t> types;

		// data_size is total amount of memory that is occupied with data items
		// (*including service fields of dataitem_t base class)
		size_t data_size = 0;
		for (unsigned n = 0; n < N; ++n) {
			size_t off_of_item = 0;
			// *Take vertices first*
			for (typename hash_and_vbuckets_t::reverse_iterator it = vbuckets.rbegin(); vbuckets.rend() != it; ++it) {
				for (typename virtual_buckets_t::const_reverse_iterator it2 = it->second.rbegin(); it->second.rend() != it2; ++it2) {
					const cluster_t &c = it2->second;
					for (unsigned i = 0; i < c.size(); ++i) {
						maps[n][clusterize[n](c[i])][(unsigned)c[i]->type_hash].push_back(off_of_item);
						types[(unsigned)c[i]->type_hash] = c[i]->type_size;
						off_of_item += c[i]->type_size;
					}
				}
			}
			if (data_size) {
				assert(data_size == off_of_item && "Amount of memory occupied by data items is the same");
			}
			data_size = off_of_item;
		}

		// Add the file header
		datatotal += sizeof(layout_file_header_begin) + sizeof(item_type_desc_t) * types.size();
		datatotal += sizeof(layout_file_header_end) + N * sizeof(list_offset_t);

		const size_t c_offset_after_headers_and_first_table = datatotal;

		std::vector<list_offset_t> lvl2_tbl;
		std::vector<list_offset_t> lvl3_tbl;

		// Offsets in each table are beginning of the table - based.
		lvl2_tbl.push_back(0);
		lvl3_tbl.push_back(0);

		size_t off;
		// Second level and 3rd level tables
		for (unsigned i = 0; i < N; ++i) {
			// maps[i].size() - number of clusters for proj i
			// + size
			
			// 2nd level table.
			// Stores pointers to 3-rd level lists.
			off = sizeof(list_t) + maps[i].size() * types.size() * sizeof(item_offset_t);
			lvl2_tbl.push_back(lvl2_tbl.back() + off);

			for (std::map<cluster_id, std::map<size_t, offsets_t> >::iterator it1 = maps[i].begin();
				maps[i].end() != it1; ++it1) {

				for (std::map<size_t, size_t>::iterator v = types.begin(); v != types.end(); ++v) {
					offsets_t &entry = (it1->second)[v->first];

					// 3rd
					// it2->second.size() - number of items in the cluster for proj i
					// + size

					// 3rd level tables.
					// Offsets to all items of the given type and from the given cluster.

					off = sizeof(list_t) + entry.size() * sizeof(item_offset_t);
					lvl3_tbl.push_back(lvl3_tbl.back() + off);
				}
			}
		}
		assert(lvl2_tbl.size() - 1 == N);

		datatotal += lvl2_tbl.back() + lvl3_tbl.back();
		
		const size_t c_offset_dataitems = datatotal;

		assert(0 == layout);
		if (0 != layout)
			free(layout);

		datatotal += data_size;
		m_datasize = data_size;
		m_dataStarts = c_offset_dataitems;
		size_t c_offset_file_end = datatotal;

		// -> 0x0
		// HEADER
		// queries[0] tbl
		// -> [c_offset_after_headers_and_first_table]
		// clusters&types[0] tbl
		// -> [c_offset_dataitems]
		// data items
		// -> [c_offset_file_end]

#ifdef _WIN32
		HANDLE hMapFile = CreateFile(mappedFile, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			0, TRUNCATE_EXISTING, 0, 0);
		if (INVALID_HANDLE_VALUE == hMapFile)
			hMapFile = CreateFile(mappedFile, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			0, CREATE_ALWAYS, 0, 0);
		HANDLE hMapping = CreateFileMapping(hMapFile, 0, PAGE_READWRITE, 0, datatotal, 0);
		layout = static_cast<char*>(MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, datatotal));
#else
		printf("m_dataSize=%u, layout.size=%u\n", m_datasize, datatotal - m_datasize);
		layout = new char[datatotal];
		
		//size_t ch = 20;
		//size_t left = 0;
		//while(left < datatotal - ch - 10) {
		//	new (layout + left) char[ch]; 
		//	left += ch;
		//}
		//layout = static_cast<char*>(malloc(datatotal));
#endif
		// Fill the file header
		layout_file_header_begin *hdr = reinterpret_cast<layout_file_header_begin*>(layout);
		hdr->fileSignature			= 0x5A4B494E;
		hdr->fileVersion			= THIS_BUILD_VERSION;
		hdr->dataitem_types_count	= types.size();
		unsigned i = 0;
		for (std::map<size_t, size_t>::iterator it = types.begin(); types.end() != it; ++it, ++i) {
			hdr->item_types[i].thash = it->first;
			hdr->item_types[i].tsize = it->second;
		}

		layout_file_header_end *hdr2 = reinterpret_cast<layout_file_header_end*>(layout +
			sizeof(layout_file_header_begin) + types.size() * sizeof(item_type_desc_t));
		hdr2->distributions_count = N;

		// Fill 1st level table - queries
		for (unsigned i = 0; i < lvl2_tbl.size() - 1; ++i) {
			hdr2->dist_list_offset[i] = c_offset_after_headers_and_first_table + lvl2_tbl[i];
		}

		// Fill 2nd level table - clusters and sub-clusters (type) for each distribution
		unsigned num, index = 0;
		for (unsigned i = 0; i < N; ++i) {
			size_t offset_to_proj_list = c_offset_after_headers_and_first_table + lvl2_tbl[i];
			num = 0;
			list_t *list = reinterpret_cast<list_t*>(layout + offset_to_proj_list);
			list->count = types.size() * maps[i].size();

			for (unsigned j = 0; j < maps[i].size(); j++) {
				for (unsigned k = 0; k < types.size(); ++k) {
					list->items[num] = c_offset_after_headers_and_first_table + lvl2_tbl.back() + lvl3_tbl[index];
					num ++;
					index++;
				}
			}
		}

		// Fill 3rd level table - lists of item offsets
		// Each list corresponds to distribution i, cluster it1 and item type it2.
		const size_t third_lvl_table_offset = c_offset_after_headers_and_first_table + lvl2_tbl.back();
		num = 0;
		for (unsigned i = 0; i < N; ++i) {
			for (std::map<cluster_id, std::map<size_t, offsets_t> >::iterator it1 = maps[i].begin();
				maps[i].end() != it1; ++it1) {

				for (std::map<size_t, offsets_t>::iterator it2 = it1->second.begin(); it1->second.end() != it2;
					++it2) {
					list_t* list = reinterpret_cast<list_t*>(layout + third_lvl_table_offset + lvl3_tbl[num]);
					list->count = it2->second.size();
					for (int b = list->count - 1; b >= 0; --b) {
						list->items[b] = it2->second[b] + c_offset_dataitems;
					}
			//		assert(sizeof(item_offset_t) * it2->second.size() + sizeof(list_t) ==
			//			lvl3_tbl[num + 1] - lvl3_tbl[num] && "Offsets in tbl3 are not correct!");
					num++;
				}
			}
		}

#ifdef _PRINT_LAYOUT
		printf("\nHOW DATA IS PLACED ON DISK...\n");
#endif
		// Copy data items
		char *data = layout + c_offset_dataitems;
		size_t off_of_item = 0;
		for (typename hash_and_vbuckets_t::reverse_iterator it = vbuckets.rbegin(); vbuckets.rend() != it; ++it) {
			for (typename virtual_buckets_t::const_reverse_iterator it2 = it->second.rbegin(); it->second.rend() != it2; ++it2) {
				const cluster_t &c = it2->second;
				for (int i = c.size() - 1; i >= 0 ; --i) {
					memcpy(data + off_of_item, c[i], c[i]->type_size);
#ifdef _PRINT_LAYOUT
					c[i]->print();
					printf("->%X \n", data + off_of_item);
#endif
					off_of_item += c[i]->type_size;
				}
			}
		}
		assert(off_of_item == c_offset_file_end - c_offset_dataitems && "Data entries size does not match to pre-calculated size");

		// Cleanup memory
		for (typename hash_and_vbuckets_t::iterator it = vbuckets.begin(); vbuckets.end() != it; ++it) {
			for (typename virtual_buckets_t::const_iterator it2 = it->second.begin(); it->second.end() != it2; ++it2) {
				const cluster_t &c = it2->second;
				for (unsigned i = 0; i < c.size(); ++i) {
					delete c[i];
				}
			}
		}
		vbuckets.clear();

#ifdef _WIN32
		UnmapViewOfFile(layout);
		CloseHandle(hMapping);
		CloseHandle(hMapFile);
		layout = 0;
#endif
	}

	void load_file() {
#ifdef _WIN32
		printf("\nLoading from file...\n");
		HANDLE hMapFile = CreateFile(mappedFile, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			0, OPEN_EXISTING, 0, 0);
		if (INVALID_HANDLE_VALUE == hMapFile) {
			assert(false && "No mapping file found...");
			return;
		}

		size_t fileSize = GetFileSize(hMapFile, 0);
		HANDLE hMapping = CreateFileMapping(hMapFile, 0, PAGE_READWRITE, 0, fileSize, 0);
		assert(layout == 0 && "layout should be zero if you load it");
		layout = static_cast<char*>(MapViewOfFile(hMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, fileSize));
#endif
		assert(0 != layout);
		layout_file_header_begin* hdr = reinterpret_cast<layout_file_header_begin*>(layout);
		m_kinds.resize(hdr->dataitem_types_count);
		for (int i = 0; i < hdr->dataitem_types_count; ++i) {
			m_types[hdr->item_types[i].thash] = hdr->item_types[i].tsize;
			m_kinds[i] = (unsigned)hdr->item_types[i].thash;
		}

		layout_file_header_end* hdr2 = reinterpret_cast<layout_file_header_end*>(reinterpret_cast<char*>(hdr) +
			sizeof(*hdr) + hdr->dataitem_types_count * sizeof(item_type_desc_t));
		
		m_maps.resize(hdr2->distributions_count);
#ifdef _PRINT_LAYOUT
		printf("\nREADING FROM DISK:\n");
#endif
		for (int i = 0; i < hdr2->distributions_count; ++i) {
			list_offset_t lvl2_list_off = hdr2->dist_list_offset[i];
			list_t *lvl2_list = reinterpret_cast<list_t*>(reinterpret_cast<char*>(layout) + lvl2_list_off);
			std::map<size_t, size_t>::iterator type_index = m_types.begin();
			for (int j = 0; j < lvl2_list->count; ++j) {

				list_offset_t lvl3_list_off = lvl2_list->items[j];
				list_t *lvl3_list = reinterpret_cast<list_t*>(reinterpret_cast<char*>(layout) + lvl3_list_off);

				for (int k = 0; k < lvl3_list->count; ++k) {

					item_offset_t item_off = lvl3_list->items[k];
					//printf("item_off=%u\n", item_off);
					dataitem_t *item = reinterpret_cast<dataitem_t*>(reinterpret_cast<char*>(layout) + item_off);
					
					//dataitem_t *newItem = (dataitem_t*)new char[type_index->second];
					//memcpy(newItem, item, type_index->second); 
					cluster_id cid = clusterize[i](item);		
					m_maps[i][(unsigned)type_index->first][cid].push_back(item/*newItem*/);
#ifdef _PRINT_LAYOUT
					item->print();
					printf(" to maps %d\n", cid); 
#endif
					// i - query no,
					// j / types.size() - cluster no
					// type_index - type no
					// k - data item no
				}

				type_index++;
				if (type_index == m_types.end())
					type_index = m_types.begin();
			}
		}

		// SORTING OF DATA ITEMS INSIDE A CLUSTER

		struct _comparator {
			bool operator() (dataitem_t *a, dataitem_t *b) {
				return (size_t)a < (size_t)b;
			}
		} comparator;

		for (unsigned i1 = 0; i1 < m_maps.size(); ++i1) {
			for (clusters_and_types_t::iterator i2 = m_maps[i1].begin(); i2 != m_maps[i1].end(); ++i2) {
				for (clusters_t::iterator i3 = i2->second.begin(); i3 != i2->second.end(); ++i3) {
					cluster_t &c = i3->second;
					std::sort(c.begin(), c.end(), comparator);
				}
			}
		}

		// SORTING OF CLUSTERS
		maps_t newmap(m_maps.size());
		for (unsigned i1 = 0; i1 < m_maps.size(); ++i1) {
			for (clusters_and_types_t::iterator i2 = m_maps[i1].begin(); i2 != m_maps[i1].end(); ++i2) {
				for (clusters_t::iterator i3 = i2->second.begin(); i3 != i2->second.end(); ++i3) {
					newmap[i1][i2->first][(size_t)i3->second[0]] = i3->second;
				}
			}
		}

		m_maps = newmap;

#ifdef _PRINT_LAYOUT
		printf("\nQUERIES\n");
		for (unsigned i1 = 0; i1 < m_maps.size(); ++i1) {
			printf("QUERY#%d\n", i1);
			for (clusters_and_types_t::iterator i2 = m_maps[i1].begin(); i2 != m_maps[i1].end(); ++i2) {
				printf("\tTYPE:%u\n", i2->first);
				for (clusters_t::iterator i3 = i2->second.begin(); i3 != i2->second.end(); ++i3) {
					printf("\t\tCLUSTER#%d\n", i3->first);
					const cluster_t &c = i3->second;
					for (unsigned i4 = 0; i4 < c.size(); ++i4) {
						printf("\t\t\t");
						c[i4]->print();
						printf("\n");
					}
				}
			}
		}
#endif

		printf("Layout loaded.\n");
	}

	template <typename Tn>
	class cluster_iterator {

	private: // const members
		cluster_t* cluster;
		const unsigned type_index;
		const unsigned query_no;
	private: // iterative members
		unsigned item_no;				// in cluster_t: 0 <= .. < cluster.size()

		void setCluster(cluster_t* _cluster) {
			cluster = _cluster;
			item_no = 0;
			if (cluster->size() == 0)
				item_no = -1;
		}

		friend class fmem<N>;
	public:
		cluster_iterator(unsigned query_nbr, unsigned item_nbr = 0) :
			item_no(item_nbr), type_index((unsigned)typeid(Tn).hash_code()), query_no(query_nbr), cluster(0) {
			// TODO: lots of checks?;
		};

		inline bool end() { return item_no == (unsigned)-1; }

		inline int size() const {
			return cluster->size();
		}

		Tn* operator()() const {
			// TODO: lots of checks
//			assert(0 != cluster.size() && "zero iterator");
			dataitem_t *di = cluster->at(item_no);
//			assert(di->type_hash == typeid(Tn).hash_code() && "Incosistent data error");
			return di->get_direct<Tn>();
		};
		void operator ++() {
			// TODO: lots of checks
//			assert(-1 != item_no && "iterator is already at the end - and can't ++");

			if (++item_no >= cluster->size())
				item_no = -1;
		};
		bool operator== (const cluster_iterator& other) {
			return item_no == other.item_no;
		}
		bool operator!= (const cluster_iterator& other) {
			return !this->operator==(other);
		}
	};

	template <typename Tn>
	class typed_iterator {

	private: // iterative members
		//clusters_t::iterator cluster_it;
		clusters_t::iterator cluster_it;
		unsigned item_no;					// in cluster_t: 0 <= .. < cluster.size()
	private: // const props of the class
		const clusters_t::const_iterator endIt;

		typed_iterator(unsigned query_nbr, clusters_t::iterator sit,
			clusters_t::iterator end_it) :
			cluster_it(sit), endIt(end_it), item_no(0){

			// End iterator
			if (cluster_it == endIt)
				return;
			// TODO: lots of checks?
			
			while (cluster_it != endIt && cluster_it->second.size() == 0) {
				this->operator++();
			}
		};
		friend class fmem<N>;
	public:
		inline bool end() { return cluster_it == endIt; } const
		Tn* operator()() const {
			// TODO: lots of checks
//			assert(item_no != -1);
//			assert(cluster_it != endIt && "iterator is out-of-range");
//			assert(0 != cluster.size() && "zero iterator");

			dataitem_t *di = cluster_it->second[item_no];
//			assert(di->type_hash == typeid(Tn).hash_code() && "Incosistent data error");
			return di->get_direct<Tn>();
		};
		void operator ++() {
			// TODO: lots of checks
			if (++item_no < cluster_it->second.size()) {
				return;
			}

			// reached the end of current cluster
			//assert(cluster_it != endIt);
			++cluster_it;
			if (cluster_it == endIt)
				return;
			
			item_no = 0;
			while (cluster_it != endIt && cluster_it->second.size() == 0) {
				// TODO: remove the recursion!
				this->operator++();
			}

		};
		bool operator== (const typed_iterator& other) {
			return cluster_it == other.cluster_it;
		}
		bool operator!= (const typed_iterator& other) {
			return !this->operator==(other);
		}
	};

	void clearMaps() {
		m_maps.clear();
		//delete layout;
	}

	template <typename Tt>
	typed_iterator<Tt> query_type(unsigned query_no = 0) {
		std::vector<size_t>::const_iterator i = std::find(m_kinds.begin(), m_kinds.end(),
			(unsigned)typeid(Tt).hash_code());
		//assert(i != super.m_kinds.end() && "Queried type not found");
		unsigned type_index = (unsigned)m_kinds[i - m_kinds.begin()];

		return typed_iterator<Tt>(query_no, m_maps[query_no][type_index].begin(),
			m_maps[query_no][type_index].end());
	}

	template <typename Tn>
	void go_to_cluster(cluster_iterator<Tn> &it, dataitem_t *obj) {
		return it.setCluster(&m_maps[it.query_no][it.type_index][clusterize[it.query_no](obj)]);
	}

	template <typename Tn>
	cluster_iterator<Tn> end(unsigned query_no = 0) {
		return cluster_iterator<Tn>(*this, -1, cluster_t(), -1);
	}

private: // VIRTUAL BUCKETING
	typedef std::map<unsigned, virtual_buckets_t> hash_and_vbuckets_t;
	hash_and_vbuckets_t vbuckets;

	std::vector<f_clusterize> clusterize;

	// After loading
	maps_t m_maps;
	std::map<size_t, size_t> m_types;
	std::vector<size_t> m_kinds;		// hashes in array
public:
	size_t m_datasize;
	size_t m_dataStarts;
private: // HELPER FUNCTIONS

	void _add_dataitem_5(dataitem_t *di) {
		assert(clusterize.size() == N && "Clusterization functions should be supplied");

		signature_t<N> signature;
		for (unsigned i = 0; i < N; ++i) {
			unsigned proj_id = clusterize[i](di);
			signature.u[i] = (proj_id / hashing);
		}
		vbuckets[di->type_hash][signature].push_back(di);
	}

private: // AUX::DERIVIATION CHECKING

	// Magic to determine at compile time that data item was not derived from base class.
	template<typename T>
	void compile_time_check_not_derived_from_dataitem_t() {
		T derived;
		enum inherited { val = (sizeof(test(derived)) == sizeof(char)) }; 
		typedef int dataitem_class_not_derived_from_dataitem_t[inherited::val?1:-1];
	}

	char test(const dataitem_t&);
	char (&test(...))[2];

private: // LAYOUT
	char *layout;
};

