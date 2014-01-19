#include "wirehair.h"
#include "Clock.hpp"
#include "AbyssinianPRNG.hpp"
using namespace cat;

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>
using namespace std;

static Clock m_clock;


// Simulation seed
const u32 SEED = 0;

// Number of trials to run
const int TRIALS = 1000;


//// Entrypoint

int main()
{
	assert(!wirehair_init());

	m_clock.OnInitialize();

	wirehair_state encoder, decoder;
	Abyssinian prng;

	// Simulate file transfers over UDP/IP
	const int block_bytes = 1300;
	u8 block[block_bytes];

	// Try each value for N
	for (int N = 2; N <= 64000; ++N)
	{
		int bytes = block_bytes * N;
		u8 *message_in = new u8[bytes];
		u8 *message_out = new u8[bytes];

		prng.Initialize(SEED);

		// Fill input message with random data
		for (int ii = 0; ii < bytes; ++ii) {
			message_in[ii] = (u8)prng.Next();
		}

		double t0 = m_clock.usec();

		// Initialize encoder
		assert(!wirehair_encode(&encoder, message_in, bytes, block_bytes));

		double t1 = m_clock.usec();

		assert(N == wirehair_count(encoder));

		double encode_time = t1 - t0;
		cout << "wirehair_encode(N = " << N << ") in " << encode_time << " usec" << endl;

		// For each trial,
		u32 overhead = 0;
		double reconstruct_time = 0;
		for (int ii = 0; ii < TRIALS; ++ii) {
			// Initialize decoder
			assert(!wirehair_decode(&decoder, bytes, block_bytes));

			assert(N == wirehair_count(decoder));

			// Simulate transmission
			int blocks_needed = 0;
			for (u32 id = 0;; ++id)
			{
				// 50% packetloss to randomize received message IDs
				if (prng.Next() & 1) continue;

				// Add a block
				++blocks_needed;

				// Write a block
				assert(!wirehair_write(encoder, id, block));

				// If decoder is ready,
				if (!wirehair_read(decoder, id, block)) {
					// If message is decoded,
					t0 = m_clock.usec();
					if (!wirehair_reconstruct(decoder, message_out)) {
						t1 = m_clock.usec();

						// Verify successful decode
						assert(!memcmp(message_in, message_out, bytes));

						// Done with transmission simulation
						break;
					}
				}
			}
			overhead += blocks_needed - N;
			reconstruct_time += t1 - t0;
		}

		cout << "wirehair_decode(N = " << N << ") average overhead = " << overhead / (double)TRIALS << " blocks, average reconstruct time = " << reconstruct_time / TRIALS << endl;

		cout << endl;

		delete []message_in;
		delete []message_out;
	}

	wirehair_free(encoder);
	wirehair_free(decoder);

	m_clock.OnFinalize();

	return 0;
}