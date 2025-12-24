#include "Brain.hpp"
#include <string>

struct NeuralNetwork : public IBrain {
    int inputSize, hiddenSize, outputSize;
    std::vector<float> weights;
    std::vector<float> biases;
    
    std::vector<float> cachedInputs;
    std::vector<float> cachedHidden;
    std::vector<float> cachedOutput;

    NeuralNetwork(int inp, int hid, int out);

    // IBrain implementation
    std::vector<float> FeedForward(const std::vector<float>& inputs) override;
    void Mutate(float rate, float strength) override;
    std::unique_ptr<IBrain> Crossover(const IBrain& other) const override;
    std::unique_ptr<IBrain> Clone() const override;
    void LearnFromReward(float reward, float learningRate) override;
    void Draw(ImVec2 pos, ImVec2 size) override;
    
    int GetInputSize() const override { return inputSize; }
    int GetOutputSize() const override { return outputSize; }
    std::string GetType() const override { return "FeedForwardNN"; }

    // Static helper for legacy/direct usage if needed, though Crossover override handles dispatch
    static NeuralNetwork CrossoverStatic(const NeuralNetwork& a, const NeuralNetwork& b);
};