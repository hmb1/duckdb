
#include "common/types/data_chunk.hpp"
#include "common/types/vector_operations.hpp"

#include "common/exception.hpp"
#include "common/helper.hpp"

using namespace duckdb;
using namespace std;

DataChunk::DataChunk()
    : count(0), column_count(0), data(nullptr), sel_vector(nullptr) {}

DataChunk::~DataChunk() {}

void DataChunk::Initialize(std::vector<TypeId> &types, bool zero_data) {
	count = 0;
	column_count = types.size();
	size_t size = 0;
	for (auto &type : types) {
		size += GetTypeIdSize(type) * STANDARD_VECTOR_SIZE;
	}
	if (size > 0) {
		owned_data = unique_ptr<char[]>(new char[size]);
		if (zero_data) {
			memset(owned_data.get(), 0, size);
		}
	}

	char *ptr = owned_data.get();
	data = unique_ptr<Vector[]>(new Vector[types.size()]);
	for (size_t i = 0; i < types.size(); i++) {
		data[i].type = types[i];
		data[i].data = ptr;
		data[i].count = 0;
		data[i].sel_vector = nullptr;

		ptr += GetTypeIdSize(types[i]) * STANDARD_VECTOR_SIZE;
	}
}

void DataChunk::Reset() {
	count = 0;
	char *ptr = owned_data.get();
	for (size_t i = 0; i < column_count; i++) {
		data[i].data = ptr;
		data[i].owns_data = false;
		data[i].count = 0;
		data[i].sel_vector = nullptr;
		data[i].owned_data = nullptr;
		data[i].string_heap.Destroy();
		ptr += GetTypeIdSize(data[i].type) * STANDARD_VECTOR_SIZE;
	}
	sel_vector = nullptr;
}

void DataChunk::Destroy() {
	data = nullptr;
	owned_data.reset();
	sel_vector = nullptr;
	column_count = 0;
	count = 0;
}

void DataChunk::Copy(DataChunk &other, size_t offset) {
	assert(column_count == other.column_count);
	other.sel_vector = nullptr;

	for (size_t i = 0; i < column_count; i++) {
		data[i].Copy(other.data[i], offset);
	}
	other.count = other.data[0].count;
}

void DataChunk::Move(DataChunk &other) {
	other.column_count = column_count;
	other.count = count;
	other.data = move(data);
	other.owned_data = move(owned_data);
	if (sel_vector) {
		other.sel_vector = sel_vector;
		if (sel_vector == owned_sel_vector) {
			// copy the owned selection vector
			memcpy(other.owned_sel_vector, owned_sel_vector,
			       STANDARD_VECTOR_SIZE * sizeof(sel_t));
		}
	}
	Destroy();
}

void DataChunk::Flatten() {
	if (!sel_vector) {
		return;
	}

	for (size_t i = 0; i < column_count; i++) {
		data[i].Flatten();
	}
	sel_vector = nullptr;
}

void DataChunk::ForceOwnership() {
	for (size_t i = 0; i < column_count; i++) {
		data[i].ForceOwnership();
	}
	sel_vector = nullptr;
	owned_data.reset();
}

void DataChunk::Append(DataChunk &other) {
	if (other.count == 0) {
		return;
	}
	if (column_count != other.column_count) {
		throw Exception("Column counts of appending chunk doesn't match!");
	}
	for (size_t i = 0; i < column_count; i++) {
		if (other.data[i].type != data[i].type) {
			throw Exception("Column types do not match!");
		}
	}
	if (count + other.count > STANDARD_VECTOR_SIZE) {
		throw Exception("Count of chunk cannot exceed STANDARD_VECTOR_SIZE!");
	}
	for (size_t i = 0; i < column_count; i++) {
		data[i].Append(other.data[i]);
	}
	count += other.count;
}

void DataChunk::MergeSelVector(sel_t *current_vector, sel_t *new_vector,
                               sel_t *result, size_t new_count) {
	for (size_t i = 0; i < new_count; i++) {
		result[i] = current_vector[new_vector[i]];
	}
}

void DataChunk::SetSelectionVector(Vector &matches) {
	if (matches.type != TypeId::BOOLEAN) {
		throw Exception(
		    "Can only set selection vector using a boolean vector!");
	}
	bool *match_data = (bool *)matches.data;
	size_t match_count = 0;
	if (sel_vector) {
		assert(matches.sel_vector);
		// existing selection vector: have to merge the selection vector
		for (size_t i = 0; i < matches.count; i++) {
			if (match_data[sel_vector[i]] && !matches.nullmask[sel_vector[i]]) {
				owned_sel_vector[match_count++] = sel_vector[i];
			}
		}
		sel_vector = owned_sel_vector;
	} else {
		// no selection vector yet: can just set the selection vector
		for (size_t i = 0; i < matches.count; i++) {
			if (match_data[i] && !matches.nullmask[i]) {
				owned_sel_vector[match_count++] = i;
			}
		}
		if (match_count < matches.count) {
			// we only have to set the selection vector if tuples were filtered
			sel_vector = owned_sel_vector;
		}
	}
	count = match_count;
	for (size_t i = 0; i < column_count; i++) {
		data[i].count = this->count;
		data[i].sel_vector = sel_vector;
	}
}

vector<TypeId> DataChunk::GetTypes() {
	vector<TypeId> types;
	for (size_t i = 0; i < column_count; i++) {
		types.push_back(data[i].type);
	}
	return types;
}

string DataChunk::ToString() const {
	string retval = "Chunk - [" + to_string(column_count) + " Columns]\n";
	for (size_t i = 0; i < column_count; i++) {
		retval += "- " + data[i].ToString() + "\n";
	}
	return retval;
}

#ifdef DEBUG
void DataChunk::Verify() {
	// verify that all vectors in this chunk have the chunk selection vector
	sel_t *v = sel_vector;
	for (size_t i = 0; i < column_count; i++) {
		assert(data[i].sel_vector == v);
	}
	// verify that all vectors in the chunk have the same count
	for (size_t i = 0; i < column_count; i++) {
		assert(count == data[i].count);
	}
}
#endif