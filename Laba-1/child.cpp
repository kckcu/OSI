#include <string>
#include <vector>
#include <unistd.h>
#include <stdlib.h>

void print_err(const std::string& msg) {
    write(STDERR_FILENO, msg.c_str(), msg.length());
}

int main() {
    char c;
    std::string num_str = "";
    std::vector<float> numbers;
    
    while (read(STDIN_FILENO, &c, 1) > 0) {
        if (c == ' ' || c == '\t' || c == '\n') {
            if (!num_str.empty()) {
                numbers.push_back(std::stof(num_str));
                num_str = "";
            }

            if (c == '\n' && !numbers.empty()) {
                float res = numbers[0];
                for (size_t i = 1; i < numbers.size(); ++i) {
                    if (numbers[i] >= -0.000001f && numbers[i] <= 0.000001f) {
                        print_err("error: division by 0\n");
                        exit(1); 
                    }
                    res /= numbers[i];
                }
                std::string out = std::to_string(res) + "\n";
                write(STDOUT_FILENO, out.c_str(), out.length()); 
                numbers.clear();
            }
        } else {
            num_str += c;
        }
    }
    
    if (!num_str.empty()) {
        numbers.push_back(std::stof(num_str));
    }
    if (!numbers.empty()) {
        float res = numbers[0];
        for (size_t i = 1; i < numbers.size(); ++i) {
            if (numbers[i] >= -0.000001f && numbers[i] <= 0.000001f) {
                print_err("error: division by 0\n");
                exit(1);
            }
            res /= numbers[i];
        }
        std::string out = std::to_string(res) + "\n";
        write(STDOUT_FILENO, out.c_str(), out.length());
    }

    exit(0);
}