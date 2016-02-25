/*
*	Evaluation of Expressions in Parallel
*	Nick Schrock
*	CIS 452 
*	2/24/2016
*/
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

pid_t g_pid;

void handler(){
	if(g_pid != 0)
		kill(g_pid, SIGUSR1);
}

void parse_expression(string expression, vector<char>& op, vector<float>& num){
	unsigned int i = 0;
	string num_string;

	const char * expr = expression.c_str();
	for(i=0; i<expression.size()-1; i++){
		//Handle Spaces
		if(expr[i] != ' '){
			//Handle operators
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
	signal(SIGINT, (_sig_func_ptr)handler);
	signal(SIGUSR1, (_sig_func_ptr)handler);
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
			g_pid = pid;
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
					printf("\nOperation #%d: 
						\nOperator Handled: %c 
						\nResult Returned= %f 
						\nProcess ID %d 
						\nParent Process ID: %d\n",loc-1, ops[i], val, getpid(), getppid());
				}
				//2. solve op num
				//next op < current op and previous op > current op
				else if(prec[index+1] < prec[index] && prec[index-1] > prec[index]){
					if(flag)
						loc++;
					val = operate(solve(ops, prec, nums, ++loc, 0), nums[i+1], ops[i]);
					printf("\nOperation #%d: 
						\nOperator Handled: %c 
						\nResult Returned= %f 
						\nProcess ID %d 
						\nParent Process ID: %d\n",loc-1, ops[i], val, getpid(), getppid());
				}
				//3. num op solve
				//next op > current op and previous op < current op
				else if(prec[index+1] > prec[index] && prec[index-1] < prec[index] ){
					if(flag)
						loc++;
					val = operate(nums[i], solve(ops, prec, nums, ++loc, 0), ops[i]);
					printf("\nOperation #%d: 
						\nOperator Handled: %c 
						\nResult Returned= %f 
						\nProcess ID %d 
						\nParent Process ID: %d\n",loc-1, ops[i], val, getpid(), getppid());
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
					printf("\nOperation #%d: 
						\nOperator Handled: %c 
						\nResult Returned= %f 
						\nProcess ID %d 
						\nParent Process ID: %d\n",loc-1, ops[i], val, getpid(), getppid());
				}

				//sleep(1);
				//while(!sig_flag);
				pause();
				write(pipefd[1], &val, sizeof(val));
				close(pipefd[1]);
				_exit(EXIT_SUCCESS);								
			}

			//Parent Process
			else{
				//signal(SIGINT, handler);
				float answer = 0;
				wait(&status);
				//if(sig_flag)
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

	return solution;
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

	printf ("Test 1 %f\n", evaluate ("2.0 * 3.0", true));
  	printf ("Test 2 %f\n", evaluate ("200.0 + 300.0", true));
  	printf ("Test 3 %f\n", evaluate ("10.0 / 5.0", true));
  	printf ("Test 4 %f\n", evaluate ("16.0 - 10.5", true));
	/*answer = 70.8 / 175.6 + 164.7 * 18.9 + 89.8 * 104.7 + 1.0 - 12455.2931891;
	printf("\n\n\n\n\nThea actual answer is: %f\n\n", answer);
	evaluate("70.8 / 175.6 + 164.7 * 18.9 + 89.8 * 104.7 + 1.0 - 12455.2931891", true);
	answer = 18.6 - 82.5 - 70.4 * 5.3 + 134.7 / 103.9 * 5.0 + 492.537805582;
	printf("\n\n\n\n\nThea actual answer is: %f\n\n", answer);
	evaluate("18.6 - 82.5 - 70.4 * 5.3 + 134.7 / 103.9 * 5.0 + 492.537805582", true);
	answer = 118.8 * 143.1 + 113.2 * 3.7 / 141.6 / 18.7 * 17.0 - 16939.9690087;
	printf("\n\n\n\n\nThea actual answer is: %f\n\n", answer);
	evaluate("118.8 * 143.1 + 113.2 * 3.7 / 141.6 / 18.7 * 17.0 - 16939.9690087", true);
	answer = 46.0 + 38.4 - 46.8 + 6.0 * 38.1 + 126.4 + 7.0 - 335.6;
	printf("\n\n\n\n\nThea actual answer is: %f\n\n", answer);
	evaluate("46.0 + 38.4 - 46.8 + 6.0 * 38.1 + 126.4 + 7.0 - 335.6;", true);
	answer = 74.1 * 191.5 + 168.9 - 182.7 / 126.5 + 171.3 * 1.0 - 14463.9057312;
	printf("\n\n\n\n\nThea actual answer is: %f\n\n", answer);
	evaluate("74.1 * 191.5 + 168.9 - 182.7 / 126.5 + 171.3 * 1.0 - 14463.90", true);
	answer = 133.8 / 117.6 * 109.8 - 132.1 * 81.0 / 165.9 + 12.0 - 6.42822268148;
	printf("\n\n\n\n\nThea actual answer is: %f\n\n", answer);
	evaluate("133.8 / 117.6 * 109.8 - 132.1 * 81.0 / 165.9 + 12.0 - 6.42822", true);
	answer = 78.1 + 112.3 - 196.8 * 78.5 - 192.7 * 26.2 / 9.0 + 15.371;
	printf("\n\n\n\n\nThea actual answer is: %f\n\n", answer);
	evaluate("78.1 + 112.3 - 196.8 * 78.5 - 192.7 * 26.2 / 9.0 + 15.37", true);
	answer = 150.6 * 127.5 * 58.7 - 6.2 / 134.7 / 42.7 + 8.0 - 112.0;
	printf("\n\n\n\n\nThea actual answer is: %f\n\n", answer);
	evaluate("150.6 * 127.5 * 58.7 - 6.2 / 134.7 / 42.7 + 8.0 - 112.0", true);
	answer = 167.0 * 145.7 / 106.8 * 57.2 / 72.9 + 30.1 / 19.0 - 111.345414371;
	printf("\n\n\n\n\nThea actual answer is: %f\n\n", answer);
	evaluate("167.0 * 145.7 / 106.8 * 57.2 / 72.9 + 30.1 / 19.0 - 111.3454", true);
	answer = 155.1 + 168.8 * 129.5 / 86.0 - 77.3 + 94.3 - 20.0 - 336.281395349;
	printf("\n\n\n\n\nThea actual answer is: %f\n\n", answer);
	evaluate("155.1 + 168.8 * 129.5 / 86.0 - 77.3 + 94.3 - 20.0 - 336.2813", true);
*/

	return 0;

}