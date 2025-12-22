#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>
#include <iostream>

struct NeuralNetwork {
    // Topology
    int inputSize;
    int hiddenSize;
    int outputSize;

    std::vector<float> weights;
    std::vector<float> biases;

    NeuralNetwork(int inputs, int hidden, int outputs) : inputSize(inputs), hiddenSize(hidden), outputSize(outputs) {
        
        // Init with random weights
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        weights.resize((inputs * hidden) + (hidden * outputs));
        biases.resize(hidden + outputs);

        for(auto& w : weights) w = dist(rng);
        for(auto& b: biases) b = dist(rng);
    }

    float activation(float x) const {
        return std::tanh(x);
    }

    std::vector<float> feedForward(const std::vector<float>& inputValues) const {
        if(inputValues.size() != inputSize) return {};

        std::vector<float> hiddenNeurons(hiddenSize);
        std::vector<float> outputs(outputSize);

        int weightIndex = 0;
        int biasIndex = 0;

        // Input layer -> hidden layer
        for (int i = 0; i < hiddenSize; ++i) {
            float sum = 0.0f;
            for(int j = 0; j < inputSize; ++j) {
                sum += inputValues[j] * weights[weightIndex++];
            }
            sum += biases[biasIndex++];
            hiddenNeurons[i] = activation(sum);
        }

        // Hidden layer -> Output layer
        for (int i = 0; i < outputSize; ++i) {
            float sum = 0.0f;
            for(int j =0 ; j < hiddenSize; ++j) {
                sum += hiddenNeurons[j] * weights[weightIndex++];
            }
                sum += biases[biasIndex++];
                outputs[i] = activation(sum);
        }
        return outputs;
    }

    void mutate(float mutationRate, float mutationStrength) {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> chance(0.0f, 1.0f);
        std::normal_distribution<float> noise(0.0f, mutationStrength);

        for(auto& w: weights) {
            if(chance(rng) < mutationRate) {
                w += noise(rng);
                w = std::clamp(w, -3.0f, 3.0f);
            }
        }
        for (auto& b : biases) {
            if (chance(rng) < mutationRate) b += noise(rng);
        }
    }
};