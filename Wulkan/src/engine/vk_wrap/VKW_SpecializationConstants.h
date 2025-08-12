#pragma once

template<typename T, uint32_t N>
class VKW_SpecializationConstants
{
public:
	VKW_SpecializationConstants() = default;
	void init(const std::array<T, N>& d, const std::array<uint32_t, N>& ids);
private:
	std::array<T, N> data;
	VkSpecializationInfo info;
	std::array<VkSpecializationMapEntry, N> map;
public:
	const VkSpecializationInfo* get_info() const { return &info; };
};

template<typename T, uint32_t N>
inline void VKW_SpecializationConstants<T, N>::init(const std::array<T, N>& d, const std::array<uint32_t, N>& ids)
{
	data = d;

	info.pData = data.data();
	info.dataSize = data.size() * sizeof(T);

	for (int i = 0; i < N; i++) {
		map.at(i).constantID = ids.at(i);
		map.at(i).offset = i * sizeof(T);
		map.at(i).size = sizeof(T);
	}
	info.pMapEntries = map.data();
	info.mapEntryCount = N;
}
