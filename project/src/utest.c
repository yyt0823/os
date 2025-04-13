#include <stdio.h>
#include <stdlib.h>

// Structure to hold the input file and corresponding expected output file
struct TestCase
{
  const char *inputFile;	// e.g., "../../A3/test-cases/tc1.txt"
  const char *expectedFile;	// e.g., "../../A3/test-cases/tc1_result.txt"
};

int
main (void)
{
  // List all your test cases here.
  struct TestCase testCases[] = {
    {"../../A3/test-cases/tc1.txt", "../../A3/test-cases/tc1_result.txt"},
    {"../../A3/test-cases/tc2.txt", "../../A3/test-cases/tc2_result.txt"},
    {"../../A3/test-cases/tc3.txt", "../../A3/test-cases/tc3_result.txt"},
    {"../../A3/test-cases/tc4.txt", "../../A3/test-cases/tc4_result.txt"},
    {"../../A3/test-cases/tc5.txt", "../../A3/test-cases/tc5_result.txt"}
  };

  // Calculate the number of test cases
  int numTests = sizeof (testCases) / sizeof (testCases[0]);
  int passedCount = 0;

  // Loop over each test case
  for (int i = 0; i < numTests; i++)
    {
      system ("make clean");
      // Compile with the appropriate parameters based on the test number.
      if (i == 0 || i == 1 || i == 3)
	{
	  printf
	    ("Compiling mysh with framesize=18 varmemsiz=10 for test %d\n",
	     i + 1);
	  system ("make mysh framesize=18 varmemsiz=10");
	}
      else if (i == 2)
	{
	  printf
	    ("Compiling mysh with framesize=21 varmemsiz=10 for test %d\n",
	     i + 1);
	  system ("make mysh framesize=21 varmemsiz=10");
	}
      else if (i == 4)
	{
	  printf
	    ("Compiling mysh with framesize=6 varmemsiz=10 for test %d\n",
	     i + 1);
	  system ("make mysh framesize=6 varmemsiz=10");
	}

      printf ("Running Test %d/%d\n", i + 1, numTests);
      printf ("  Input:    %s\n", testCases[i].inputFile);
      printf ("  Expected: %s\n", testCases[i].expectedFile);

      // Run the shell program with the test input, redirecting output to output.txt
      char command[512];
      snprintf (command, sizeof (command), "./mysh < %s > output.txt",
		testCases[i].inputFile);
      system (command);

      // Compare output.txt with the expected output using diff
      char diffCmd[512];
      snprintf (diffCmd, sizeof (diffCmd), "diff output.txt %s",
		testCases[i].expectedFile);
      int result = system (diffCmd);

      // Check the result of diff
      if (result == 0)
	{
	  printf
	    ("  \033[32mTest PASSED: Output matches expected result.\033[0m\n\n");
	  passedCount++;
	}
      else
	{
	  printf
	    ("  \033[0;31mTest FAILED: Output differs from expected result.\033[0m\n\n");
	}

      // Clean build artifacts before the next test
      printf ("Cleaning build artifacts.\n\n");
      system ("make clean");
    }

  // Print a summary of how many tests passed
  printf ("Summary: %d out of %d tests passed.\n", passedCount, numTests);

  return 0;
}
