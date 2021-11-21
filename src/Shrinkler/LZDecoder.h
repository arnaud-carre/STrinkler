// Copyright 1999-2014 Aske Simon Christensen. See LICENSE.txt for usage terms.

/*

Decoder for the LZ encoder.

*/

#pragma once

#include "Decoder.h"
#include "LZEncoder.h"

class LZReceiver {
public:
	virtual bool receiveLiteral(unsigned char value) = 0;
	virtual bool receiveReference(int offset, int length) = 0;
	virtual ~LZReceiver() {}
};

class LZVerifier : public LZReceiver, public CompressedDataReadListener {
	int hunk;
	unsigned char *data;
	int data_length;
	int hunk_mem;
	int pos;

	unsigned char getData(int i) {
		if (data == NULL || i >= data_length) return 0;
		return data[i];
	}

public:
	int compressed_longword_count;
	int front_overlap_margin;

	LZVerifier(int hunk, unsigned char *data, int data_length, int hunk_mem) : hunk(hunk), data(data), data_length(data_length), hunk_mem(hunk_mem), pos(0) {
		compressed_longword_count = 0;
		front_overlap_margin = 0;
	}

	bool receiveLiteral(unsigned char lit) {
		if (pos >= hunk_mem) {
			printf("Verify error: literal at position %d in hunk %d overflows hunk!\n",
				pos, hunk);
			return false;
		}
		if (lit != getData(pos)) {
			printf("Verify error: literal at position %d in hunk %d has incorrect value (0x%02X, should be 0x%02X)!\n",
				pos, hunk, lit, getData(pos));
			return false;
		}
		pos += 1;
		return true;
	}

	bool receiveReference(int offset, int length) {
		if (offset < 1 || offset > pos) {
			printf("Verify error: reference at position %d in hunk %d has invalid offset (%d)!\n",
				pos, hunk, offset);
			return false;
		}
		if (length > hunk_mem - pos) {
			printf("Verify error: reference at position %d in hunk %d overflows hunk (length %d, %d bytes past end)!\n",
				pos, hunk, length, pos + length - hunk_mem);
			return false;
		}
		for (int i = 0; i < length; i++) {
			if (getData(pos - offset + i) != getData(pos + i)) {
				printf("Verify error: reference at position %d in hunk %d has incorrect value for byte %d of %d (0x%02X, should be 0x%02X)!\n",
					pos, hunk, i, length, getData(pos - offset + i), getData(pos + i));
				return false;
			}
		}
		pos += length;
		return true;
	}

	int size() {
		return pos;
	}

	void read(int index) {
		// Another longword of compresed data read
		int margin = pos - compressed_longword_count * 4;
		if (margin > front_overlap_margin) {
			front_overlap_margin = margin;
		}
		compressed_longword_count += 1;
	}
};

class LZDecoder {
	Decoder *decoder;

	int decode(int context) const {
		return decoder->decode(LZEncoder::NUM_SINGLE_CONTEXTS + context);
	}

	int decodeNumber(int context_group) const {
		return decoder->decodeNumber(LZEncoder::NUM_SINGLE_CONTEXTS + (context_group << 8));
	}

public:
	LZDecoder(Decoder *decoder) : decoder(decoder) {

	}

	bool decode(LZReceiver& receiver) {
		bool ref = false;
		bool prev_was_ref = false;
		int pos = 0;
		int offset = 0;
		do {
			if (ref) {
				bool repeated = false;
				if (!prev_was_ref) {
					repeated = decode(LZEncoder::CONTEXT_REPEATED);
				}
				if (!repeated) {
					offset = decodeNumber(LZEncoder::CONTEXT_GROUP_OFFSET) - 2;
					if (offset == 0) break;
				}
				int length = decodeNumber(LZEncoder::CONTEXT_GROUP_LENGTH);
				if (!receiver.receiveReference(offset, length)) return false;
				pos += length;
				prev_was_ref = true;
			} else {
				int parity = pos & 1;
				int context = 1;
				for (int i = 7 ; i >= 0 ; i--) {
					int bit = decode((parity << 8) | context);
					context = (context << 1) | bit;
				}
				unsigned char lit = context;
				if (!receiver.receiveLiteral(lit)) return false;
				pos += 1;
				prev_was_ref = false;
			}
			int parity = pos & 1;
			ref = decode(LZEncoder::CONTEXT_KIND + (parity << 8));
		} while (true);
		return true;
	}

};
