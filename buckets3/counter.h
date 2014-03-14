/*
	Counter of memory locality
*/
#pragma once

#include <vector>

#define PROXIMITY_SIZE		64

static struct _scounter {
	void ref(size_t addr) {
		for (stream_d_t &sdt : streams) {
			if (addr - sdt.last_address < 0)
				continue;
			if (addr - sdt.last_address > PROXIMITY_SIZE)
				continue;
			sdt.last_address = addr;
			sdt.accesses_number ++;
			return;
		}
		stream_d_t new_sdt;
		new_sdt.last_address = addr;
		new_sdt.accesses_number = 1;
		streams.push_back(new_sdt);
		printf("// <-------- NEW STREAM DETECTED!!\n");
	}

	void print_summary() {
		printf("\nSTREAMS SUMMARY:\n");
		printf("number: %d\n", streams.size());
		long double avg = 0.0f;
		for (stream_d_t &sdt : streams) {
			avg += sdt.accesses_number;
		}
		avg /= streams.size();
		printf("(*unfair*)avg accesses per stream: %lf\n", avg);
	}

private:
	struct stream_d_t {
		size_t last_address;
		unsigned accesses_number;

		stream_d_t() : last_address((size_t)-1), accesses_number(0) {};
	};

	std::vector<stream_d_t> streams;

} scounter;