#pragma once
#include <vector>

struct NeuralNetwork {
    int inputSize, hiddenSize, outputSize;
    std::vector<float> weights;
    std::vector<float> biases;
    
    std::vector<float> cachedInputs;
    std::vector<float> cachedHidden;
    std::vector<float> cachedOutput;

    NeuralNetwork(int inp, int hid, int out);

    std::vector<float> FeedForward(const std::vector<float>& inputs);
    void Mutate(float rate, float strength);
    
    void Backpropagate(const std::vector<float>& targetOutput, float learningRate);
    void LearnFromReward(float reward, float learningRate);
    
    static NeuralNetwork Crossover(const NeuralNetwork& a, const NeuralNetwork& b);
};