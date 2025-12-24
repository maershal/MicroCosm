#include "RNNBrain.hpp"
#include "Config.hpp"
#include "imgui.h"
#include <cmath>
#include <algorithm>

RNNBrain::RNNBrain(int inp, int hid, int out) 
    : inputSize(inp), hiddenSize(hid), outputSize(out) {
    
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    auto& rng = GetRNG();
    
    inputWeights.resize(inp * hid);
    recurrentWeights.resize(hid * hid);
    outputWeights.resize(hid * out);
    biases.resize(hid);
    nextHidden.resize(hid);
    
    for (auto& w : inputWeights) w = dist(rng);
    for (auto& w : recurrentWeights) w = dist(rng);
    for (auto& w : outputWeights) w = dist(rng);
    for (auto& b : biases) b = dist(rng);
    
    ResetState();
}

void RNNBrain::ResetState() {
    hiddenState.assign(hiddenSize, 0.0f);
}

std::vector<float> RNNBrain::FeedForward(const std::vector<float>& inputs) {
    cachedInputs = inputs;
    std::fill(nextHidden.begin(), nextHidden.end(), 0.0f);
    std::vector<float> output(outputSize, 0.0f);
    
    int wRec = 0;
    int wInp = 0;
    
    // Calculate new hidden state
    for (int h = 0; h < hiddenSize; ++h) {
        float sum = biases[h];
        for (int i = 0; i < inputSize; ++i) sum += inputs[i] * inputWeights[wInp++];
        for (int ph = 0; ph < hiddenSize; ++ph) sum += hiddenState[ph] * recurrentWeights[wRec++];
        nextHidden[h] = std::tanh(sum);
    }
    
    hiddenState = nextHidden;
    
    int wOut = 0;
    for (int o = 0; o < outputSize; ++o) {
        float sum = 0.0f;
        for (int h = 0; h < hiddenSize; ++h) sum += hiddenState[h] * outputWeights[wOut++];
        output[o] = std::tanh(sum);
    }
    
    return output;
}

void RNNBrain::Mutate(float rate, float strength) {
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    std::normal_distribution<float> noise(0.0f, strength);
    auto& rng = GetRNG();

    for (auto& w : inputWeights) if (chance(rng) < rate) w = std::clamp(w + noise(rng), -3.0f, 3.0f);
    for (auto& w : recurrentWeights) if (chance(rng) < rate) w = std::clamp(w + noise(rng), -3.0f, 3.0f);
    for (auto& w : outputWeights) if (chance(rng) < rate) w = std::clamp(w + noise(rng), -3.0f, 3.0f);
    for (auto& b : biases) if (chance(rng) < rate) b = std::clamp(b + noise(rng), -3.0f, 3.0f);
}

std::unique_ptr<IBrain> RNNBrain::Clone() const {
    return std::make_unique<RNNBrain>(*this);
}

std::unique_ptr<IBrain> RNNBrain::Crossover(const IBrain& other) const {
    const auto* otherRNN = dynamic_cast<const RNNBrain*>(&other);
    if (otherRNN) {
        return std::make_unique<RNNBrain>(CrossoverStatic(*this, *otherRNN));
    }
    // Cross-Architecture Fallback: 50% chance to be RNN (cloned+mutated), 50% to be the other type (cloned+mutated)
    if (RandomFloat(0,1) < 0.5f) {
        auto child = this->Clone();
        child->Mutate(0.5f, 0.5f); // High mutation for hybridization
        return child;
    } else {
        auto child = other.Clone();
        child->Mutate(0.5f, 0.5f); // High mutation
        return child;
    }
}

RNNBrain RNNBrain::CrossoverStatic(const RNNBrain& a, const RNNBrain& b) {
    RNNBrain child = a;
    std::uniform_int_distribution<int> coin(0, 1);
    auto& rng = GetRNG();
    
    for (size_t i = 0; i < child.inputWeights.size(); ++i) 
        child.inputWeights[i] = coin(rng) ? a.inputWeights[i] : b.inputWeights[i];
    for (size_t i = 0; i < child.recurrentWeights.size(); ++i) 
        child.recurrentWeights[i] = coin(rng) ? a.recurrentWeights[i] : b.recurrentWeights[i];
    for (size_t i = 0; i < child.outputWeights.size(); ++i) 
        child.outputWeights[i] = coin(rng) ? a.outputWeights[i] : b.outputWeights[i];
    for (size_t i = 0; i < child.biases.size(); ++i) 
        child.biases[i] = coin(rng) ? a.biases[i] : b.biases[i];
        
    return child;
}

void RNNBrain::LearnFromReward(float reward, float learningRate) {
    // RNN lifetime learning is complex, leaving as no-op or simple accumulation 
    // to avoid instability in this simple implementation.
}

void RNNBrain::Draw(ImVec2 pos, ImVec2 size) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    
    float nodeRadius = 8.0f;
    float layerSpacing = size.x / 3.0f;
    
    int inputCount = inputSize;
    float inputSpacing = size.y / (inputCount + 1);
    std::vector<ImVec2> inputNodes;
    for (int i = 0; i < inputCount; ++i) {
        ImVec2 nodePos(pos.x, pos.y + inputSpacing * (i + 1));
        inputNodes.push_back(nodePos);
        draw->AddCircleFilled(nodePos, nodeRadius, IM_COL32(100, 200, 255, 200));
    }
    
    int hiddenCount = hiddenSize;
    float hiddenSpacing = size.y / (hiddenCount + 1);
    std::vector<ImVec2> hiddenNodes;
    for (int i = 0; i < hiddenCount; ++i) {
        ImVec2 nodePos(pos.x + layerSpacing, pos.y + hiddenSpacing * (i + 1));
        hiddenNodes.push_back(nodePos);
        draw->AddCircleFilled(nodePos, nodeRadius, IM_COL32(255, 200, 100, 200));
        
        // Draw recurrent loop indicator (small circle above node)
        draw->AddCircle(ImVec2(nodePos.x, nodePos.y - 12), 6.0f, IM_COL32(255, 255, 0, 150));
    }
    
    int outputCount = outputSize;
    float outputSpacing = size.y / (outputCount + 1);
    std::vector<ImVec2> outputNodes;
    for (int i = 0; i < outputCount; ++i) {
        ImVec2 nodePos(pos.x + layerSpacing * 2, pos.y + outputSpacing * (i + 1));
        outputNodes.push_back(nodePos);
        draw->AddCircleFilled(nodePos, nodeRadius, IM_COL32(100, 255, 150, 200));
    }
    
    // Input -> Hidden
    int wIdx = 0;
    for (int h = 0; h < hiddenCount; ++h) {
        for (int i = 0; i < inputCount; ++i) {
            float w = inputWeights[wIdx++];
            ImU32 color = w > 0 ? IM_COL32(100, 255, 100, 100) : IM_COL32(255, 100, 100, 100);
            float thickness = std::abs(w) * 2.0f;
            draw->AddLine(inputNodes[i], hiddenNodes[h], color, thickness);
        }
    }
    
    // Hidden -> Output
    wIdx = 0;
    for (int o = 0; o < outputCount; ++o) {
        for (int h = 0; h < hiddenCount; ++h) {
            float w = outputWeights[wIdx++];
            ImU32 color = w > 0 ? IM_COL32(100, 255, 100, 100) : IM_COL32(255, 100, 100, 100);
            float thickness = std::abs(w) * 2.0f;
            draw->AddLine(hiddenNodes[h], outputNodes[o], color, thickness);
        }
    }
}
