#pragma once
#include "Brain.hpp"
#include "NEATGenome.hpp"
#include <map>

struct NEATBrain : public IBrain {
    Genome genome;
    int inputSize, outputSize;
    
    // Fast lookup for computation
    // We rebuild this when genome changes
    // Adjacency list approach or topologically sorted list
    struct FastNode {
        int id;
        NodeType type;
        float value = 0.0f;
        float bias = 0.0f;
        float incomingSum = 0.0f;
        std::vector<std::pair<int, float>> incoming; // index in fastNodes vector, weight
    };
    std::vector<FastNode> fastNetwork;
    std::map<int, int> idToIndex; // Map NodeID -> fastNetwork Index
    
    NEATBrain(int inp, int out) : inputSize(inp), outputSize(out) {
        genome.Initialize(inp, out);
        RebuildNetwork();
    }
    
    NEATBrain(const Genome& g, int inp, int out) : genome(g), inputSize(inp), outputSize(out) {
        RebuildNetwork();
    }
    
    void RebuildNetwork() {
        fastNetwork.clear();
        idToIndex.clear();
        
        // 1. Create FastNodes
        for(const auto& gene : genome.nodes) {
            FastNode fn;
            fn.id = gene.id;
            fn.type = gene.type;
            fn.bias = gene.bias;
            fastNetwork.push_back(fn);
        }
        
        // Sort: Sensors -> Hidden -> Output (roughly, but simple sort works if basic)
        // Topological sort is better for feed-forward correctness
        // Simple heuristic sort by Type then ID or X
        std::sort(fastNetwork.begin(), fastNetwork.end(), [](const FastNode& a, const FastNode& b) {
             if(a.type != b.type) return (int)a.type < (int)b.type; // Sensor(0) < Hidden(1) < Output(2)
             return a.id < b.id;
        });
        
        // Map IDs
        for(size_t i=0; i<fastNetwork.size(); ++i) {
            idToIndex[fastNetwork[i].id] = i;
        }
        
        // 2. Link Connections
        for(const auto& con : genome.connections) {
            if(!con.enabled) continue;
            if(idToIndex.find(con.inNode) == idToIndex.end() || idToIndex.find(con.outNode) == idToIndex.end()) continue;
            
            int outIdx = idToIndex[con.outNode];
            int inIdx = idToIndex[con.inNode];
            
            fastNetwork[outIdx].incoming.push_back({inIdx, con.weight});
        }
    }

    std::vector<float> FeedForward(const std::vector<float>& inputs) override {
        // Reset
        for(auto& node : fastNetwork) {
            node.value = 0.0f; 
            node.incomingSum = 0.0f;
        }
        
        // Set Inputs
        int inputCount = 0;
        for(auto& node : fastNetwork) {
            if(node.type == NodeType::Sensor) {
                if(inputCount < (int)inputs.size()) node.value = inputs[inputCount++];
            }
        }
        
        // Propagate
        // Since we sorted Sensor -> Hidden -> Output, simple iteration works for strictly feed-forward
        // However, with arbitrary mutations, we might have recurrent loops or unordered hidden layers.
        // For strictly feedforward, we need depth-sort. For now, multiple passes or simple iteration (generalized)
        
        // Generalized approach: Compute sums, then activate. Steps?
        // Let's do a single pass because we assume feed-forward X-sorted structure mostly.
        
        for(auto& node : fastNetwork) {
            if(node.type == NodeType::Sensor) continue;
            
            float sum = node.bias;
            for(auto& link : node.incoming) {
                // link.first is index
                sum += fastNetwork[link.first].value * link.second;
            }
            node.value = std::tanh(sum);
        }
        
        // Collect Outputs
        std::vector<float> outputs;
        for(auto& node : fastNetwork) {
            if(node.type == NodeType::Output) {
                outputs.push_back(node.value);
            }
        }
        
        // Fill remaining if needed (shouldn't happen)
        while((int)outputs.size() < outputSize) outputs.push_back(0.0f);
        return outputs;
    }
    
    void Mutate(float rate, float strength) override {
        (void)strength;
        // NEAT mutations with specific probabilities
        genome.MutateWeight(0.8f * rate, 0.5f); // 80% chance to mutate weights? scale by rate
        genome.MutateAddConnection(0.05f * rate); // 5% chance
        genome.MutateAddNode(0.03f * rate); // 3% chance
        RebuildNetwork();
    }
    
    std::unique_ptr<IBrain> Crossover(const IBrain& other) const override {
        const auto* otherNeat = dynamic_cast<const NEATBrain*>(&other);
        if (otherNeat) {
            Genome babyG = Genome::Crossover(this->genome, otherNeat->genome);
            return std::make_unique<NEATBrain>(babyG, inputSize, outputSize);
        }
        return Clone();
    }
    
    std::unique_ptr<IBrain> Clone() const override {
        return std::make_unique<NEATBrain>(genome, inputSize, outputSize);
    }
    
    void LearnFromReward(float reward, float learningRate) override {
        // NEAT generally doesn't use backprop lifetime learning standardly
        // Could implement Hebbian or simple weight nudge
        // Simplified: Nudge last activated weights? Too complex for now.
        (void)reward; (void)learningRate;
    }
    
    int GetInputSize() const override { return inputSize; }
    int GetOutputSize() const override { return outputSize; }
    std::string GetType() const override { return "NEAT"; }
    
    void Draw(ImVec2 pos, ImVec2 size) override {
        auto* draw = ImGui::GetWindowDrawList();
        
        // Map logical X/Y to Screen X/Y
        auto GetScreenPos = [&](float nmX, float nmY) {
            return ImVec2(pos.x + nmX * size.x, pos.y + nmY * size.y);
        };
        
        // Draw connections
        for(const auto& con : genome.connections) {
            if(!con.enabled) continue;
            
            // Find nodes
            float x1=0, y1=0, x2=0, y2=0;
            bool f1=false, f2=false;
            for(const auto& n : genome.nodes) {
                if(n.id == con.inNode) { x1=n.x; y1=n.y; f1=true; }
                if(n.id == con.outNode) { x2=n.x; y2=n.y; f2=true; }
            }
            if(!f1 || !f2) continue;
            
            ImU32 col = (con.weight > 0) ? IM_COL32(100, 255, 100, 150) : IM_COL32(255, 100, 100, 150);
            float thickness = std::min(5.0f, std::max(1.0f, std::abs(con.weight) * 2.0f));
            draw->AddLine(GetScreenPos(x1, y1), GetScreenPos(x2, y2), col, thickness);
        }
        
        // Draw nodes
        for(const auto& n : genome.nodes) {
            ImU32 col = IM_COL32(200, 200, 200, 255);
            if(n.type == NodeType::Sensor) col = IM_COL32(100, 200, 255, 255);
            if(n.type == NodeType::Output) col = IM_COL32(100, 255, 100, 255);
            
            draw->AddCircleFilled(GetScreenPos(n.x, n.y), 6.0f, col);
        }
    }
};
