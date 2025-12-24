#include "NeuralNetwork.hpp"
#include "Config.hpp"
#include <cmath>
#include <algorithm>

#include "imgui.h"

NeuralNetwork::NeuralNetwork(int inp, int hid, int out) 
    : inputSize(inp), hiddenSize(hid), outputSize(out) {
    
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    weights.resize((inp * hid) + (hid * out));
    biases.resize(hid + out);
    auto& rng = GetRNG();
    for (auto& w : weights) w = dist(rng);
    for (auto& b : biases) b = dist(rng);
    
    // Initialize caches
    cachedInputs.resize(inp);
    cachedHidden.resize(hid);
    cachedOutput.resize(out);
}

std::vector<float> NeuralNetwork::FeedForward(const std::vector<float>& inputs) {
    cachedInputs = inputs;
    
    int wIdx = 0;
    int bIdx = 0;

    // Input -> Hidden
    for (int i = 0; i < hiddenSize; ++i) {
        float sum = biases[bIdx++];
        for (int j = 0; j < inputSize; ++j) {
            sum += inputs[j] * weights[wIdx++];
        }
        cachedHidden[i] = std::tanh(sum);
    }

    // Hidden -> Output
    for (int i = 0; i < outputSize; ++i) {
        float sum = biases[bIdx++];
        for (int j = 0; j < hiddenSize; ++j) {
            sum += cachedHidden[j] * weights[wIdx++];
        }
        cachedOutput[i] = std::tanh(sum);
    }
    return cachedOutput;
}

void NeuralNetwork::Mutate(float rate, float strength) {
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    std::normal_distribution<float> noise(0.0f, strength);
    auto& rng = GetRNG();

    for (auto& w : weights) if (chance(rng) < rate) w = std::clamp(w + noise(rng), -3.0f, 3.0f);
    for (auto& b : biases)  if (chance(rng) < rate) b = std::clamp(b + noise(rng), -3.0f, 3.0f);
}

void NeuralNetwork::LearnFromReward(float reward, float learningRate) {
    if (cachedInputs.empty() || cachedOutput.empty()) return;
    
    // Simple reinforcement learning: adjust weights based on reward
    // This uses a simplified approximation of gradients
    
    std::vector<float> targetOutput = cachedOutput;
    
    for (int i = 0; i < outputSize; ++i) {
        if (reward > 0) {
            targetOutput[i] = cachedOutput[i] + reward * 0.1f;
        } else {
            targetOutput[i] = cachedOutput[i] + reward * 0.05f;
        }
        targetOutput[i] = std::clamp(targetOutput[i], -1.0f, 1.0f);
    }
    
    // Backprop the target
    // We implement Backprop locally since it's specific to this implementation
    
    // Backprop implementation inline
    std::vector<float> outputGradients(outputSize);
    std::vector<float> hiddenGradients(hiddenSize);

    // Output gradients
    for (int i = 0; i < outputSize; ++i) {
        float error = targetOutput[i] - cachedOutput[i];
        float tanhDerivative = 1.0f - cachedOutput[i] * cachedOutput[i];
        outputGradients[i] = error * tanhDerivative;
    }

    // Hidden gradients
    int wIdx = inputSize * hiddenSize; 
    for (int h = 0; h < hiddenSize; ++h) {
        float error = 0.0f;
        for (int o = 0; o < outputSize; ++o) {
            error += outputGradients[o] * weights[wIdx + o * hiddenSize + h];
        }
        float tanhDerivative = 1.0f - cachedHidden[h] * cachedHidden[h];
        hiddenGradients[h] = error * tanhDerivative;
    }

    // Update weights/biases Hidden->Output
    wIdx = inputSize * hiddenSize;
    int bIdx = hiddenSize;
    for (int o = 0; o < outputSize; ++o) {
        for (int h = 0; h < hiddenSize; ++h) {
            weights[wIdx++] += learningRate * outputGradients[o] * cachedHidden[h];
        }
        biases[bIdx++] += learningRate * outputGradients[o];
    }

    // Update weights/biases Input->Hidden
    wIdx = 0;
    bIdx = 0;
    for (int h = 0; h < hiddenSize; ++h) {
        for (int i = 0; i < inputSize; ++i) {
            weights[wIdx++] += learningRate * hiddenGradients[h] * cachedInputs[i];
        }
        biases[bIdx++] += learningRate * hiddenGradients[h];
    }

    // Clamp
    for (auto& w : weights) w = std::clamp(w, -5.0f, 5.0f);
    for (auto& b : biases) b = std::clamp(b, -5.0f, 5.0f);
}

std::unique_ptr<IBrain> NeuralNetwork::Clone() const {
    return std::make_unique<NeuralNetwork>(*this);
}

std::unique_ptr<IBrain> NeuralNetwork::Crossover(const IBrain& other) const {
    if (auto* otherNN = dynamic_cast<const NeuralNetwork*>(&other)) {
        return std::make_unique<NeuralNetwork>(CrossoverStatic(*this, *otherNN));
    }
    return Clone();
}

NeuralNetwork NeuralNetwork::CrossoverStatic(const NeuralNetwork& a, const NeuralNetwork& b) {
    NeuralNetwork child = a; 
    std::uniform_int_distribution<int> coin(0, 1);
    auto& rng = GetRNG();
    
    for (size_t i = 0; i < child.weights.size(); ++i) child.weights[i] = coin(rng) ? a.weights[i] : b.weights[i];
    for (size_t i = 0; i < child.biases.size(); ++i)  child.biases[i] = coin(rng) ? a.biases[i] : b.biases[i];
    return child;
}

void NeuralNetwork::Draw(ImVec2 pos, ImVec2 size) {
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
    }
    
    int outputCount = outputSize;
    float outputSpacing = size.y / (outputCount + 1);
    std::vector<ImVec2> outputNodes;
    for (int i = 0; i < outputCount; ++i) {
        ImVec2 nodePos(pos.x + layerSpacing * 2, pos.y + outputSpacing * (i + 1));
        outputNodes.push_back(nodePos);
        draw->AddCircleFilled(nodePos, nodeRadius, IM_COL32(100, 255, 150, 200));
    }
    
    int wIdx = 0;
    for (int h = 0; h < hiddenCount; ++h) {
        for (int i = 0; i < inputCount; ++i) {
            float w = weights[wIdx++];
            ImU32 color = w > 0 ? IM_COL32(100, 255, 100, 100) : IM_COL32(255, 100, 100, 100);
            float thickness = std::abs(w) * 2.0f;
            draw->AddLine(inputNodes[i], hiddenNodes[h], color, thickness);
        }
    }
    
    for (int o = 0; o < outputCount; ++o) {
        for (int h = 0; h < hiddenCount; ++h) {
            float w = weights[wIdx++];
            ImU32 color = w > 0 ? IM_COL32(100, 255, 100, 100) : IM_COL32(255, 100, 100, 100);
            float thickness = std::abs(w) * 2.0f;
            draw->AddLine(hiddenNodes[h], outputNodes[o], color, thickness);
        }
    }
}