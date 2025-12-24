#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include <map>
#include "Config.hpp"
#include "Brain.hpp"

// Forward declarations
struct Genome;

// --- Innovation Tracking ---
// Keeps track of global innovations to align historical markings
class InnovationCounter {
public:
    static int GetInnovation(int inNode, int outNode) {
        static int currentInnovation = 0;
        static std::map<std::pair<int, int>, int> history;
        
        std::pair<int, int> connection = {inNode, outNode};
        if (history.find(connection) == history.end()) {
            history[connection] = ++currentInnovation;
        }
        return history[connection];
    }
    
    static int GetNextNodeId() {
        static int currentNodeId = 1000; // Start high to avoid conflict with initial sensor/output ids
        return ++currentNodeId;
    }
};

// --- Gene Structures ---

enum class NodeType { Sensor, Hidden, Output };

struct NodeGene {
    int id;
    NodeType type;
    float bias;
    
    // For visualization/sorting
    float x = 0.0f; 
    float y = 0.0f;
    
    explicit NodeGene(int _id, NodeType _t) : id(_id), type(_t) {
        bias = RandomFloat(-3.0f, 3.0f);
    }
    
    NodeGene(const NodeGene& other) = default;
};

struct ConnectionGene {
    int inNode;
    int outNode;
    float weight;
    bool enabled;
    int innovation; // Historical marking
    
    ConnectionGene(int in, int out, float w, bool en, int innov)
        : inNode(in), outNode(out), weight(w), enabled(en), innovation(innov) {}
};

// --- Genome ---

struct Genome {
    std::vector<NodeGene> nodes;
    std::vector<ConnectionGene> connections;
    int layers = 2; // For drawing optimization
    
    Genome() = default;
    
    // Initialize standard fully connected feed-forward (or empty)
    void Initialize(int inputs, int outputs) {
        nodes.clear();
        connections.clear();
        
        // Add Input Nodes
        for(int i=0; i<inputs; ++i) {
            NodeGene n(i, NodeType::Sensor);
            n.x = 0.1f;
            n.y = (float)(i + 1) / (inputs + 1);
            nodes.push_back(n);
        }
        
        // Add Output Nodes
        for(int i=0; i<outputs; ++i) {
            NodeGene n(inputs + i, NodeType::Output); // IDs continue after inputs
            n.x = 0.9f;
            n.y = (float)(i + 1) / (outputs + 1);
            nodes.push_back(n);
        }
        
        // Initial connections (Fully connected? Or sparse?)
        // Let's start with sparse - 30% density
        for(int i=0; i<inputs; ++i) {
            for(int j=0; j<outputs; ++j) {
                if(RandomFloat(0,1) < 0.5f) { // 50% density
                    int inId = i;
                    int outId = inputs + j;
                    int innov = InnovationCounter::GetInnovation(inId, outId);
                    connections.emplace_back(inId, outId, RandomFloat(-2.0f, 2.0f), true, innov);
                }
            }
        }
    }
    
    // --- Mutations ---
    
    void MutateWeight(float rate, float power) {
        for(auto& con : connections) {
            if(RandomFloat(0,1) < rate) {
                if(RandomFloat(0,1) < 0.1f) {
                    con.weight = RandomFloat(-3.0f, 3.0f); // New random weight
                } else {
                    con.weight += RandomFloat(-power, power); // Slight nudge
                }
                con.weight = std::clamp(con.weight, -10.0f, 10.0f);
            }
        }
    }
    
    void MutateAddConnection(float rate) {
        if(RandomFloat(0,1) > rate) return;
        
        // Try to find two nodes to connect
        if(nodes.empty()) return;
        
        int attempts = 20;
        while(attempts-- > 0) {
            int idx1 = (int)RandomFloat(0, nodes.size());
            int idx2 = (int)RandomFloat(0, nodes.size());
            
            NodeGene& n1 = nodes[idx1];
            NodeGene& n2 = nodes[idx2];
            
            // Rules:
            // 1. Can't connect output to input (standard NEAT usually feed-forward, but recurrent is allowed in advanced NEAT)
            // 2. Can't connect if connection exists
            // 3. Prevent loops if we want strict feed-forward (complex check), let's allow recurrent for coolness?
            // Let's stick to simple rules: From Left(x) to Right(x)
            
            if(n1.type == NodeType::Output && n2.type == NodeType::Output) continue;
            if(n1.type == NodeType::Sensor && n2.type == NodeType::Sensor) continue;
             // Ensure flow direction (or allow recurrent?) - Let's enforce x1 < x2 for FeedForward
             // If x1 >= x2, it's recurrent. Let's Allow it but separate later if needed.
             // For sim stability, let's enforce non-recurrent for now:
            if(n1.x >= n2.x) continue; 
            
            bool exists = false;
            for(const auto& con : connections) {
                if(con.inNode == n1.id && con.outNode == n2.id) {
                    exists = true; break;
                }
            }
            
            if(!exists) {
                int innov = InnovationCounter::GetInnovation(n1.id, n2.id);
                connections.emplace_back(n1.id, n2.id, RandomFloat(-2.0f, 2.0f), true, innov);
                return;
            }
        }
    }
    
    void MutateAddNode(float rate) {
         if(RandomFloat(0,1) > rate) return;
         if(connections.empty()) return;
         
         // Pick random enabled connection
         int conIdx = -1;
         int attempts = 10;
         while(attempts-- > 0) {
             int idx = (int)RandomFloat(0, connections.size());
             if(connections[idx].enabled) {
                 conIdx = idx;
                 break;
             }
         }
         
         if(conIdx == -1) return;
         
         ConnectionGene& con = connections[conIdx];
         con.enabled = false; // Disable old connection
         
         int inNodeId = con.inNode;
         int outNodeId = con.outNode;
         
         // New Node
         int newNodeId = InnovationCounter::GetNextNodeId();
         NodeGene newNode(newNodeId, NodeType::Hidden);
         
         // Position logic (for drawing)
         // Find inNode and outNode objects to get x/y
         // Optimized: just scan list once (slow but clear)
         float inX=0, inY=0, outX=1, outY=1;
         for(auto& n : nodes) { if(n.id == inNodeId) {inX=n.x; inY=n.y;} if(n.id == outNodeId) {outX=n.x; outY=n.y;} }
         
         newNode.x = (inX + outX) / 2.0f;
         newNode.y = (inY + outY) / 2.0f + RandomFloat(-0.1f, 0.1f);
         nodes.push_back(newNode);
         
         // Two new connections
         // 1. In -> New (Weight = 1.0)
         int innov1 = InnovationCounter::GetInnovation(inNodeId, newNodeId);
         connections.emplace_back(inNodeId, newNodeId, 1.0f, true, innov1);
         
         // 2. New -> Out (Weight = OldWeight)
         int innov2 = InnovationCounter::GetInnovation(newNodeId, outNodeId);
         connections.emplace_back(newNodeId, outNodeId, con.weight, true, innov2);
    }
    
    // --- Crossover ---
    static Genome Crossover(const Genome& mom, const Genome& dad) {
        Genome baby;
        baby.nodes = mom.nodes; // Inherit nodes (topologically, usually from most fit. Assuming mom is more fit or equal)
        // Note: Real NEAT node inheritance is complex. Simplification: Inherit nodes from fit parent.
        
        // Matching genes
        auto& momGenes = mom.connections;
        auto& dadGenes = dad.connections; // Assuming const ref
        
        // Use maps for O(N) alignment (or just loop since genomes are small)
        // Rely on innovation numbers
        
        // We iterate through all innovations found in both
        size_t m=0, d=0;
        // Sort first? connections should be chronological usually, but let's sort to be safe
        std::vector<ConnectionGene> mSorted = momGenes;
        std::vector<ConnectionGene> dSorted = dadGenes;
        std::sort(mSorted.begin(), mSorted.end(), [](auto& a, auto& b){ return a.innovation < b.innovation; });
        std::sort(dSorted.begin(), dSorted.end(), [](auto& a, auto& b){ return a.innovation < b.innovation; });
        
        // Assume mom is fitter (or same). Disjoint/Excess genes from mom are inherited.
        // Disjoint/Excess from dad are discarded (unless equal fitness - tricky).
        
        while(m < mSorted.size() && d < dSorted.size()) {
            if(mSorted[m].innovation == dSorted[d].innovation) {
                // Matching
                ConnectionGene gene = (rand()%2) ? mSorted[m] : dSorted[d];
                baby.connections.push_back(gene);
                m++; d++;
            } else if(mSorted[m].innovation < dSorted[d].innovation) {
                // Disjoint in Mom
                baby.connections.push_back(mSorted[m]);
                m++;
            } else {
                // Disjoint in Dad - discard (assuming Mom is fitter) 
                d++;
            }
        }
        
        // Excess in Mom
        while(m < mSorted.size()) {
            baby.connections.push_back(mSorted[m]);
            m++;
        }
        
        // Validate Nodes: The specific nodes referenced by connections must exist in baby.nodes.
        // Simplified: Baby copies Mom's nodes. If Dad had disjoint connections with *new* nodes, 
        // we might have dangling connections if we just copy Mom's nodes.
        // Handling this properly requires union of nodes or inheriting nodes from the parent acting as source for connections.
        // Fix: Add any missing nodes referenced by inherited connections.
        
        for(const auto& con : baby.connections) {
            bool inFound = false;
            bool outFound = false;
            for(const auto& n : baby.nodes) {
                if(n.id == con.inNode) inFound = true;
                if(n.id == con.outNode) outFound = true;
            }
            if(!inFound) {
                 // Fetch node from parents... slow
                 for(const auto& n : dad.nodes) if(n.id == con.inNode) baby.nodes.push_back(n); // Copy from dad
            }
            if(!outFound) {
                 for(const auto& n : dad.nodes) if(n.id == con.outNode) baby.nodes.push_back(n);
            }
        }
        
        return baby;
    }
};
