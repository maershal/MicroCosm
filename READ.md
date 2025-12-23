# MicroCosm

**MicroCosm** is an evolutionary simulation representing an ecosystem where autonomous agents, controlled by simple neural networks, learn to survive through interaction with their environment.

---

## System Architecture

### 1. Agents
Each agent possesses:
* **Physical Attributes:** Position (Vector2), rotation angle, gender, and energy level.
* **Brain:** A simple feed-forward neural network.
* **Metabolism:** The agent's energy decreases over time; if it reaches zero, the agent dies.
* **Statistics:** Lifespan, fruits eaten, poisons avoided, and number of offspring. These are used to calculate the **fitness score**.



### 2. Environment
* **Fruits:** Provide energy.
* **Poison:** Deducts energy.
* These objects are **dynamically renewable** (respawnable).

### 3. Optimization: Spatial Grid
To ensure efficient collision detection and target searching, the world is divided into a **Spatial Grid**. Instead of calculating the distance to every fruit on the map, an agent only searches neighboring grid cells. This significantly increases performance with large populations.



---

## Operating Mechanisms

### 1. Sensory System (Perception)
Agents do not see the entire world. Their "vision" is based on a `SensorData` structure that feeds the neural network with:
* Distance and angle to the nearest fruit.
* Distance and angle to the nearest poison.

### 2. Control (Neural Network)
* The neural network has **5 inputs and 2 outputs**. 
* Outputs are interpreted as the speed of a **differential (tank) drive system** (`leftTrack`, `rightTrack`).
* The difference between outputs causes rotation, while the average determines forward/backward movement speed.

### 3. Reproduction
The simulation uses a sexual reproduction model. Breeding occurs when:
* A male and a female meet.
* Both have energy levels exceeding **120 units**.
* The offspring receives a brain resulting from a **crossover** of the parents' genes, plus an additional **mutation**.

---

## Evolutionary Algorithm

### 1. Fitness Function
An agent's success is calculated using the following formula:

$$Fitness = (Lifespan \times 0.3) + (Offspring \times 15) + (Fruits \times 2) + (PoisonsAvoided \times 0.5)$$

### 2. Selection and New Generation
When a population goes extinct, the system creates a new generation based on the **Top 30** individuals from previous cycles:
* **Elite:** The best agents pass to the next generation unchanged.
* **Mutation:** Offspring of the best individuals with light or strong weight mutations.
* **New Blood:** A few random agents are added to maintain genetic diversity.

---

## Proposed Improvements and Development

* **Advanced Algorithms:** Replacing fixed networks with the **NEAT algorithm**, Recurrent Neural Networks (RNNs), or implementing **backpropagation** for learning during an agent's lifetime.
* **Phenotypes:** Implementing trade-offs (e.g., higher speed requires higher energy consumption).
* **Obstacles:** Static walls and mazes to force spatial navigation learning.
* **Communication:**
    * *Output:* Pheromone level (0-1).
    * *Input:* "Are there agents with strong pheromones nearby?"
    * Allows agents to warn of poison or call for mates.
* **Energy Inheritance:** Offspring receiving a portion of energy from the mother.
* **Combat:** Aggressive individuals and fighting mechanics.
* **Specialized Species:**
    * **Herbivores:** Bonuses for eating fruits.
    * **Scavengers:** Reduced penalties for consuming poison.
    * **Predators:** Ability to "hunt" other agents (stealing energy upon contact).
* **Analytics:** Live charts showing trends in average fitness and lifespan across generations.
* **Health System:** Disease and immunity mechanics.