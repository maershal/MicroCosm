#include "NeuralNetwork.hpp"
#include "Config.hpp"
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
    
    // Initialize caches
    cachedInputs.resize(inp);
    cachedHidden.resize(hid);
    cachedOutput.resize(out);
}

std::vector<float> NeuralNetwork::FeedForward(const std::vector<float>& inputs) {
    cachedInputs = inputs;
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
        cachedHidden[i] = hidden[i];
    }

    // Hidden -> Output
    for (int i = 0; i < outputSize; ++i) {
        float sum = biases[bIdx++];
        for (int j = 0; j < hiddenSize; ++j) {
            sum += hidden[j] * weights[wIdx++];
        }
        output[i] = std::tanh(sum);
        cachedOutput[i] = output[i];
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

void NeuralNetwork::Backpropagate(const std::vector<float>& targetOutput, float learningRate) {
    if (cachedInputs.empty()) return; // No forward pass cached
    
    std::vector<float> outputGradients(outputSize);
    std::vector<float> hiddenGradients(hiddenSize);
    
    // Calculate output layer gradients
    for (int i = 0; i < outputSize; ++i) {
        float error = targetOutput[i] - cachedOutput[i];
        // Derivative of tanh
        float tanhDerivative = 1.0f - cachedOutput[i] * cachedOutput[i];
        outputGradients[i] = error * tanhDerivative;
    }
    
    // Calculate hidden layer gradients
    int wIdx = inputSize * hiddenSize; // Start at hidden->output weights
    for (int h = 0; h < hiddenSize; ++h) {
        float error = 0.0f;
        for (int o = 0; o < outputSize; ++o) {
            error += outputGradients[o] * weights[wIdx + o * hiddenSize + h];
        }
        float tanhDerivative = 1.0f - cachedHidden[h] * cachedHidden[h];
        hiddenGradients[h] = error * tanhDerivative;
    }
    
    // Update weights and biases: hidden -> output
    wIdx = inputSize * hiddenSize;
    int bIdx = hiddenSize;
    for (int o = 0; o < outputSize; ++o) {
        for (int h = 0; h < hiddenSize; ++h) {
            weights[wIdx++] += learningRate * outputGradients[o] * cachedHidden[h];
        }
        biases[bIdx++] += learningRate * outputGradients[o];
    }
    
    // Update weights and biases: input -> hidden
    wIdx = 0;
    bIdx = 0;
    for (int h = 0; h < hiddenSize; ++h) {
        for (int i = 0; i < inputSize; ++i) {
            weights[wIdx++] += learningRate * hiddenGradients[h] * cachedInputs[i];
        }
        biases[bIdx++] += learningRate * hiddenGradients[h];
    }
    
    // Clamp weights to prevent explosion
    for (auto& w : weights) w = std::clamp(w, -5.0f, 5.0f);
    for (auto& b : biases) b = std::clamp(b, -5.0f, 5.0f);
}

//Reward-based learning (reinforcement-style)
void NeuralNetwork::LearnFromReward(float reward, float learningRate) {
    if (cachedInputs.empty() || cachedOutput.empty()) return;
    
    // Use reward to create target outputs that are slightly biased
    // towards actions that led to positive rewards
    std::vector<float> targetOutput = cachedOutput;
    
    for (int i = 0; i < outputSize; ++i) {
        // Push outputs in the direction of the reward
        if (reward > 0) {
            targetOutput[i] = cachedOutput[i] + reward * 0.1f;
        } else {
            targetOutput[i] = cachedOutput[i] + reward * 0.05f;
        }
        targetOutput[i] = std::clamp(targetOutput[i], -1.0f, 1.0f);
    }
    
    Backpropagate(targetOutput, learningRate);
}

NeuralNetwork NeuralNetwork::Crossover(const NeuralNetwork& a, const NeuralNetwork& b) {
    NeuralNetwork child = a; 
    std::uniform_int_distribution<int> coin(0, 1);
    auto& rng = GetRNG();
    
    for (size_t i = 0; i < child.weights.size(); ++i) child.weights[i] = coin(rng) ? a.weights[i] : b.weights[i];
    for (size_t i = 0; i < child.biases.size(); ++i)  child.biases[i] = coin(rng) ? a.biases[i] : b.biases[i];
    return child;
}