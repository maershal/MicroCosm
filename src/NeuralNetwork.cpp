#include "NeuralNetwork.hpp"
#include "Config.hpp" // Potrzebne dla RandomFloat/GetRNG
#include <cmath>
#include <algorithm>

NeuralNetwork::NeuralNetwork(int inp, int hid, int out) 
    : inputSize(inp), hiddenSize(hid), outputSize(out) {
    
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    weights.resize((inp * hid) + (hid * out));
    biases.resize(hid + out);
    auto& rng = GetRNG();
    for (auto& w : weights) w = dist(rng);
    for (auto& b : biases) b = dist(rng);
}

std::vector<float> NeuralNetwork::FeedForward(const std::vector<float>& inputs) const {
    std::vector<float> hidden(hiddenSize, 0.0f);
    std::vector<float> output(outputSize, 0.0f);
    
    int wIdx = 0;
    int bIdx = 0;

    // Input -> Hidden
    for (int i = 0; i < hiddenSize; ++i) {
        float sum = biases[bIdx++];
        for (int j = 0; j < inputSize; ++j) {
            sum += inputs[j] * weights[wIdx++];
        }
        hidden[i] = std::tanh(sum);
    }

    // Hidden -> Output
    for (int i = 0; i < outputSize; ++i) {
        float sum = biases[bIdx++];
        for (int j = 0; j < hiddenSize; ++j) {
            sum += hidden[j] * weights[wIdx++];
        }
        output[i] = std::tanh(sum);
    }
    return output;
}

void NeuralNetwork::Mutate(float rate, float strength) {
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    std::normal_distribution<float> noise(0.0f, strength);
    auto& rng = GetRNG();

    for (auto& w : weights) if (chance(rng) < rate) w = std::clamp(w + noise(rng), -3.0f, 3.0f);
    for (auto& b : biases)  if (chance(rng) < rate) b = std::clamp(b + noise(rng), -3.0f, 3.0f);
}

NeuralNetwork NeuralNetwork::Crossover(const NeuralNetwork& a, const NeuralNetwork& b) {
    NeuralNetwork child = a; 
    std::uniform_int_distribution<int> coin(0, 1);
    auto& rng = GetRNG();
    
    for (size_t i = 0; i < child.weights.size(); ++i) child.weights[i] = coin(rng) ? a.weights[i] : b.weights[i];
    for (size_t i = 0; i < child.biases.size(); ++i)  child.biases[i] = coin(rng) ? a.biases[i] : b.biases[i];
    return child;
}
