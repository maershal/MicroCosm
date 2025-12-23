#pragma once
#include <vector>

struct NeuralNetwork {
    int inputSize, hiddenSize, outputSize;
    std::vector<float> weights;
    std::vector<float> biases;

    NeuralNetwork(int inp, int hid, int out);

    std::vector<float> FeedForward(const std::vector<float>& inputs) const;
    void Mutate(float rate, float strength);
    
    static NeuralNetwork Crossover(const NeuralNetwork& a, const NeuralNetwork& b);
};
