#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>

using namespace std;

void parse_expression(string expression, vector<char>& op, vector<float>& num){
	unsigned int i = 0;
	string num_string;

	const char * expr = expression.c_str();
	for(i=0; i<expression.size()-1; i++){
		//Handle Spaces
		if(expr[i] != ' '){
			//Handle operatorsr
			if(expr[i] == '+' || expr[i] == '-' || expr[i] == '/' || expr[i] == '*'){
				op.push_back(expr[i]);
			}
			//Handle numbers
			else{
				while(expr[i] != ' '){
					if(expr[i] == '\0'){
						break;
					}
					num_string += expr[i];
					i++;
				}
				i--;
				num.push_back(atof(num_string.c_str()));
				num_string.clear();
			}
		}
	}
}

/*void print_arrays(vector<char>& operators, vector<int>& precedence, vector<float>& nums){
	int i=0;
	printf("\n\n Printing Arrays...\n");
	//Print Operators Array
	printf("\nOperators: ");
	printf("\nSize = %d\n", operators.size());
	for(i=0; i<operators.size(); i++){
		printf("%c ", operators[i]);
	}

	//Print Precedence array
	printf("\n\nPrecedence: ");
	printf("\nSize = %d\n", precedence.size());
	for(i=0; i<precedence.size(); i++){
		printf("%d ", precedence[i]);
	}

	//Print Numbers array
	printf("\n\nNums: ");
	printf("\nSize = %d\n", nums.size());
	for(i=0; i<nums.size(); i++){
		printf("%f ", nums[i]);
	}
	printf("\n\n");

}*/


void get_precedence(vector<char>& ops, vector<int>& pres){
	unsigned int i;
	int count = 0;
	pres.clear();
	for(i=0; i<ops.size()+1; i++){
		pres.push_back(0);
	}
	pres[0] = -1;
	for(i=ops.size()+1; i>=1; i--){
		if(ops[i-1] == '-' || ops[i-1] == '+'){
			pres[i] = count;
			count++;  
		}
	}
	for(i=ops.size()+1; i>=1; i--){
		if(ops[i-1] == '*' || ops[i-1] == '/'){
			pres[i] = count;
			count++;  
			
		}
	}
	pres.push_back(-1);
}

float operate(float x, float y, char op){
	float answer = 0;
	switch(op){
		case '+':
			answer = x + y;
			break;
		case '-':
			answer = x - y;
			break;
		case '*':
			answer = x*y;
			break;
		case '/':
			answer = x/y;
			break;
	}

	return answer;
}



float solve(vector<char>& ops, vector<int>& prec, vector<float>& nums, int loc, int flag){
	//printf("\nsolve\n");
	pid_t pid;
	int pipefd[2];
	int status;
	unsigned int i;
	int index;
	
	//Loop through each operaotr
	for(i=0; i<ops.size(); i++){
		index = i+1;
		//If this is the operator we are looking for... fork.  
		//'loc' tells us the precedence of operator we are looking for  
		if(prec[index] == loc){
			//create pipe
			if(pipe(pipefd) == -1){
				perror("pipe");
				exit(EXIT_FAILURE);
			}
			//Create child process
			pid = fork();
			if(pid < 0){
				perror("Fork Failure");
				exit(EXIT_FAILURE);
			}
			//Child Process
			else if(pid == 0){
				float val;
				close(pipefd[0]);				

				//1. num op num
				//next op < current op and previous op < current op
				if(prec[index+1] < prec[index] && prec[index-1] < prec[index] ){
					val = operate(nums[i], nums[i+1], ops[i]);
					loc++;
					//printf("\nCase 1: op%d %c val= %f Process ID %d\n",loc, ops[i], val, getpid());
				}
				//2. solve op num
				//next op < current op and previous op > current op
				else if(prec[index+1] < prec[index] && prec[index-1] > prec[index]){
					if(flag)
						loc++;
					val = operate(solve(ops, prec, nums, ++loc, 0), nums[i+1], ops[i]);
					//printf("\nCase 2: op%d %c val= %f Process ID %d\n",loc, ops[i], val, getpid());
				}
				//3. num op solve
				//next op > current op and previous op < current op
				else if(prec[index+1] > prec[index] && prec[index-1] < prec[index] ){
					if(flag)
						loc++;
					val = operate(nums[i], solve(ops, prec, nums, ++loc, 0), ops[i]);
					//printf("\nCase 3: op%d %c val= %f Process ID %d\n",loc, ops[i], val, getpid());
				}
				//4. solve op solve
				//next op and previous op > current op
				else{
					if(flag)
						loc++;
					loc++;
					float op1 = solve(ops, prec, nums, loc, 1);
					loc++;
					float op2 = solve(ops, prec, nums, loc, 0);
					val = operate(op1, op2, ops[i]);
					//printf("\nCase 4: op%d %c val= %f Process ID %d\n",loc-1, ops[i], val, getpid());
				}

				write(pipefd[1], &val, sizeof(val));
				close(pipefd[1]);
				_exit(EXIT_SUCCESS);								
			}

			//Parent Process
			else{
				float answer = 0;
				wait(&status);
				close(pipefd[1]);
				read(pipefd[0], &answer, sizeof(answer));

				close(pipefd[0]);

				return answer;
			}
		}
	}
	return 0;
}



float evaluate (string expr, bool immediate_result) {

	float solution;

	vector<float> nums;
	vector<char> operators;
	vector<int> precedence;

	//Parse Expression into an array of numbers and an array of operators
	parse_expression(expr, operators, nums);

	get_precedence(operators, precedence);

	cout << expr << endl;
	
	solution = solve(operators, precedence, nums, 0, 0);

	printf("\nSolution Found: %f\n", solution);

	return 0;
  }


int main(){

	
	double answer;

	answer = 5.0 + 45.0 * 3.0 - 2.0 / 4.0;
	printf("\n\n\n\n\nThe actual answer is: %f\n\n", answer);
	evaluate("5.0 + 45.0 * 3.0 - 2.0 / 4.0", true);

	answer = 5.0 + 45.0 - 23.0 * 24.0 / 3.0;
	printf("\n\n\n\n\nThe actual answer is: %f\n\n", answer);
	evaluate("5.0 + 45.0 - 23.0 * 24.0 / 3.0", true);

	answer = 2.0 * 3.0;
	printf("\n\n\n\n\nThe actual answer is: %f\n\n", answer);
	evaluate("2.0 * 3.0", true);

	answer = 2.0 + 3.0 * 4.0;
	printf("\n\n\n\n\nThe actual answer is: %f\n\n", answer);
	evaluate("2.0 + 3.0 * 4.0", true);

	answer = 2.0 * 3.0 + 4.0 / 5.0;
	printf("\n\n\n\n\nThe actual answer is: %f\n\n", answer);
	evaluate("2.0 * 3.0 + 4.0 / 5.0", true);

	answer = 2.0 + 3.0 * 4.0 - 5.0 / 6.0;
	printf("\n\n\n\n\nThe actual answer is: %f\n\n", answer);
	evaluate("2.0 + 3.0 * 4.0 - 5.0 / 6.0", true);

	answer = 2.0 + 3.0 + 4.0 + 5.0;
	printf("\n\n\n\n\nThe actual answer is: %f\n\n", answer);
	evaluate("2.0 + 3.0 + 4.0 + 5.0", true);

	return 0;

}