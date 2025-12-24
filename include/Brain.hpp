#pragma once
#include <vector>
#include <memory>
#include "imgui.h" 

struct IBrain {
    virtual ~IBrain() = default;

    // Core functionality
    virtual std::vector<float> FeedForward(const std::vector<float>& inputs) = 0;
    
    // Genetic Algorithm operators
    virtual void Mutate(float rate, float strength) = 0;
    virtual std::unique_ptr<IBrain> Crossover(const IBrain& other) const = 0;
    virtual std::unique_ptr<IBrain> Clone() const = 0;
    
    // Optional learning (Backprop/RL)
    virtual void LearnFromReward(float reward, float learningRate) = 0;

    // Visualization
    virtual void Draw(ImVec2 pos, ImVec2 size) = 0;
    
    // Inspection
    virtual int GetInputSize() const = 0;
    virtual int GetOutputSize() const = 0;
    virtual std::string GetType() const = 0;
};
