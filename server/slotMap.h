#include <vector>
#include <cinttypes>
#include <optional>
#include <functional>
#include "manager.h"

#ifndef SERVER_SLOTMAP_H
#define SERVER_SLOTMAP_H

class SlotMap {
	typedef uint8_t Generation;
	typedef uint8_t MajorIndex;
	typedef uint16_t MinorIndex;

public:
	class Key {
		friend SlotMap;
		Generation g;
		MajorIndex M;
		MinorIndex m;

	private:
		Key() {}
		Key(Generation g, MajorIndex M, MinorIndex m) : g(g), M(M), m(m) {}

	public:
		Key(uint32_t u) : g(u), M(u >> (8 * (sizeof(g) + sizeof(m)))), m(u >> (8 * sizeof(g))) {}
		uint32_t gameId() {
			return (static_cast<uint32_t>(M) << ((sizeof(g) + sizeof(m)) * 8))
					| (static_cast<uint32_t>(m) << (sizeof(g) * 8))
					| (static_cast<uint32_t>(g));
		}
	};

private:
	std::vector<std::vector<std::tuple<Manager, Generation>>> managers;
	std::vector<std::vector<Key>> freeSlots;

	void addManagerSet() {
		managers.emplace_back().resize(1U << (8 * sizeof(MinorIndex)));
		auto &keys = freeSlots.emplace_back();
		keys.reserve(sizeof(MinorIndex));
		constexpr unsigned secondTierSize = (1U << (8 * sizeof(MinorIndex)));
		for (unsigned i = 0; i < secondTierSize; i++) {
			keys.push_back(Key(0, managers.size() - 1, secondTierSize - 1 - i));
		}
	}

	void reclaim(Key key) {
		key.g++;
		freeSlots[key.M].push_back(key);

        auto &outer = managers[key.M];
        std::get<1>(outer[key.m])++;
	}

public:
	Key getSlot() {
		if (freeSlots.back().size() == 0) {
			addManagerSet();
		}
		auto it = std::find_if(freeSlots.begin(), freeSlots.end(), [](auto &x) { return x.size() > 0; });
		Key key;
		if (it == freeSlots.end()) {
			addManagerSet();
			auto &v = freeSlots.back();
			key = v.back();
			v.pop_back();
		} else {
			key = it->back();
			it->pop_back();
		}

		Manager &manager = (*this)[key].value();
		new (&manager) Manager;
		auto self = this;
		manager.setDeleter([self, key]() {
			Manager &m = (*self)[key].value();
			m.~Manager();
			self->reclaim(key);
		});
		return key;
	}

	std::optional<std::reference_wrapper<Manager>> operator[](Key key) {
		if (key.M >= managers.size()) {
			return {};
		}
		auto &outer = managers[key.M];
		if (key.m >= outer.size()) {
			return {};
		}
		auto &[manager, generation] = outer[key.m];
		if (generation != key.g) {
			return {};
		}
		return std::optional(std::reference_wrapper(manager));
	}

	SlotMap() {
		addManagerSet();
	}
};

#endif //SERVER_SLOTMAP_H
