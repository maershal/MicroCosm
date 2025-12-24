#pragma once
#include "Brain.hpp"
#include <string>

struct RNNBrain : public IBrain {
    int inputSize, hiddenSize, outputSize;
    
    // Weights
    std::vector<float> inputWeights;  // Input -> Hidden
    std::vector<float> recurrentWeights; // Hidden(t-1) -> Hidden(t)
    std::vector<float> outputWeights; // Hidden -> Output
    std::vector<float> biases; // For Hidden layer
    
    // State
    std::vector<float> hiddenState; // Current hidden state
    std::vector<float> cachedInputs; // For visual/debug

    RNNBrain(int inp, int hid, int out);

    // IBrain implementation
    std::vector<float> FeedForward(const std::vector<float>& inputs) override;
    void Mutate(float rate, float strength) override;
    std::unique_ptr<IBrain> Crossover(const IBrain& other) const override;
    std::unique_ptr<IBrain> Clone() const override;
    void LearnFromReward(float reward, float learningRate) override; // Simplified for now
    void Draw(ImVec2 pos, ImVec2 size) override;
    
    int GetInputSize() const override { return inputSize; }
    int GetOutputSize() const override { return outputSize; }
    std::string GetType() const override { return "RecurrentNN"; }
    
    void ResetState();

private:
    static RNNBrain CrossoverStatic(const RNNBrain& a, const RNNBrain& b);
};
