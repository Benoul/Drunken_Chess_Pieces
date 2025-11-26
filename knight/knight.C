#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <numeric>
#include <array>

class RandomWalkSimulator {
private:
    std::mt19937 rng;
    
    // this is how knights like to move it
    inline static constexpr std::array<std::array<int, 2>, 8> movements = {{
        {{1, 2}}, {{-1, 2}}, {{1, -2}}, {{-1, -2}},
        {{2, 1}}, {{2, -1}}, {{-2, 1}}, {{-2, -1}}
    }};

public:
    RandomWalkSimulator(unsigned int seed) : rng(seed) {}
    
    // looking for the last position of the walk, that is all we care about here
    std::array<double, 2> randomWalkOnlyLast(double bias, int time) {
        if (time <= 1) {
            return {0.0, 0.0};
        }
        
        // I have chosen one movement to have a slight bias
        double p_1 = (1.0/8.0) + bias;
        double p_others = (1.0/8.0) - bias/7.0;
        
        std::vector<double> probs = {p_1, p_others, p_others, p_others, 
                                     p_others, p_others, p_others, p_others};
        
        std::discrete_distribution<int> distribution(probs.begin(), probs.end());
        
        std::array<double, 2> position = {0.0, 0.0};
        
        for (int i = 0; i < time - 1; ++i) {
            int step_index = distribution(rng);
            position[0] += movements[step_index][0];
            position[1] += movements[step_index][1];
        }
        
        return position;
    }
    
    // speaks for iteself
    double Linregress(const std::vector<double>& x, const std::vector<double>& y) {
        size_t n = x.size();
        
        double x_mean = std::accumulate(x.begin(), x.end(), 0.0) / n;
        double y_mean = std::accumulate(y.begin(), y.end(), 0.0) / n;
        
        double numerator = 0.0;
        double denominator = 0.0;
        
        for (size_t i = 0; i < n; ++i) {
            numerator += (x[i] - x_mean) * (y[i] - y_mean);
            denominator += (x[i] - x_mean) * (x[i] - x_mean);
        }
        
        return numerator / denominator;
    }
    
    // finding the slope of the log log graph
    double findslope(double bias = 0.1, int time_range = 100, int N = 10) {
        std::vector<double> r_meansquare;
        std::vector<double> t;
        
        for (int time = 1; time < time_range; ++time) {
            t.push_back(time);
            std::vector<double> r_temp;
            
            for (int i = 0; i < N; ++i) {
                auto final_position = randomWalkOnlyLast(bias, time);
                double r_squared = final_position[0] * final_position[0] + 
                                  final_position[1] * final_position[1];
                r_temp.push_back(r_squared);
            }
            
            double mean_r_square = std::accumulate(r_temp.begin(), r_temp.end(), 0.0) / N;
            r_meansquare.push_back(mean_r_square);
        }
        
        // skip first element to avoid log(0), that tends to not work very well...
        std::vector<double> log_r_rms;
        std::vector<double> log_t;
        
        for (size_t i = 1; i < r_meansquare.size(); ++i) {
            double r_rms = std::sqrt(r_meansquare[i]);
            log_r_rms.push_back(std::log(r_rms));
            log_t.push_back(std::log(t[i]));
        }
        
        return Linregress(log_t, log_r_rms);
    }
};

int main() {
    std::random_device rd;
    unsigned int seed = rd() % 100 + 1;
    
    RandomWalkSimulator simulator(seed);
    double slope = simulator.findslope();
    
    std::cout << "Slope: " << slope << std::endl;
    
    return 0;
}
