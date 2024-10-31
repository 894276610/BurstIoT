#pragma once
#include <vector>

class Combiner {
public:
    Combiner(const std::vector<std::vector<int>>& arrays)
        : arrays(arrays), indices(arrays.size(), 0), finished(false) {}

    std::vector<int> getNext() {
        if (finished) return {}; // 返回空向量表示已完成

        std::vector<int> result;
        for (size_t i = 0; i < arrays.size(); ++i) {
            result.push_back(arrays[i][indices[i]]);
        }

        // 更新索引
        size_t i = arrays.size() - 1;
        while (i < arrays.size() && ++indices[i] >= arrays[i].size()) {
            indices[i] = 0;
            if (i == 0) {
                finished = true; // 完成所有组合
                break;
            }
            --i;
        }
        return result;
    }

    bool isFinished() const {
        return finished;
    }

private:
    std::vector<std::vector<int>> arrays;
    std::vector<size_t> indices;
    bool finished;
};