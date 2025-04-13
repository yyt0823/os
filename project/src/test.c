#include <stdio.h>
#include <stdlib.h>

// Structure to hold the input file and corresponding expected output file
struct TestCase {
    const char *inputFile;      // e.g., "../../A2/test-cases/T_source.txt"
    const char *expectedFile;   // e.g., "../../A2/test-cases/T_source_result.txt"
};

int main (void) {
    // List all your test cases here. 
    // Each line is { "input_file", "expected_output_file" }.
    struct TestCase testCases[] = {
        {"../../A2/test-cases/T_source.txt",
         "../../A2/test-cases/T_source_result.txt"},
        {"../../A2/test-cases/T_FCFS.txt",
         "../../A2/test-cases/T_FCFS_result.txt"},
        {"../../A2/test-cases/T_FCFS2.txt",
         "../../A2/test-cases/T_FCFS2_result.txt"},
        {"../../A2/test-cases/T_FCFS3.txt",
         "../../A2/test-cases/T_FCFS3_result.txt"},
        {"../../A2/test-cases/T_FCFS4.txt",
         "../../A2/test-cases/T_FCFS4_result.txt"},
        {"../../A2/test-cases/T_RR.txt",
         "../../A2/test-cases/T_RR_result.txt"},
        {"../../A2/test-cases/T_RR2.txt",
         "../../A2/test-cases/T_RR2_result.txt"},
        {"../../A2/test-cases/T_RR3.txt",
         "../../A2/test-cases/T_RR3_result.txt"},
        {"../../A2/test-cases/T_RR4.txt",
         "../../A2/test-cases/T_RR4_result.txt"},
        {"../../A2/test-cases/T_SJF.txt",
         "../../A2/test-cases/T_SJF_result.txt"},
        {"../../A2/test-cases/T_SJF2.txt",
         "../../A2/test-cases/T_SJF2_result.txt"},
        {"../../A2/test-cases/T_SJF3.txt",
         "../../A2/test-cases/T_SJF3_result.txt"},
        {"../../A2/test-cases/T_SJF4.txt",
         "../../A2/test-cases/T_SJF4_result.txt"},
        {"../../A2/test-cases/T_AGING.txt",
         "../../A2/test-cases/T_AGING_result.txt"},
        {"../../A2/test-cases/T_AGING2.txt",
         "../../A2/test-cases/T_AGING2_result.txt"},
        {"../../A2/test-cases/T_AGING3.txt",
         "../../A2/test-cases/T_AGING3_result.txt"},
        {"../../A2/test-cases/T_AGING4.txt",
         "../../A2/test-cases/T_AGING4_result2.txt"},
        {"../../A2/test-cases/T_RR30.txt",
         "../../A2/test-cases/T_RR30_result.txt"},
        {"../../A2/test-cases/T_RR30_2.txt",
         "../../A2/test-cases/T_RR30_2_result.txt"},
        {"../../A2/test-cases/T_background.txt",
         "../../A2/test-cases/T_background_result.txt"}
        // Add or remove test pairs as needed
    };

    // Calculate the number of test cases
    int numTests = sizeof (testCases) / sizeof (testCases[0]);

    // Counter for how many tests passed
    int passedCount = 0;

    // Loop over each test case
    for (int i = 0; i < numTests; i++) {
        printf ("Running Test %d/%d\n", i + 1, numTests);
        printf ("  Input:    %s\n", testCases[i].inputFile);
        printf ("  Expected: %s\n", testCases[i].expectedFile);

        // 1) Run the shell program with the test input, redirecting output to output.txt
        char command[512];
        snprintf (command, sizeof (command), "./mysh < %s > output.txt",
                  testCases[i].inputFile);
        system (command);

        // 2) Compare output.txt with the expected output using diff
        char diffCmd[512];
        snprintf (diffCmd, sizeof (diffCmd), "diff output.txt %s",
                  testCases[i].expectedFile);
        int result = system (diffCmd);

        // 3) Check the result of diff
        if (result == 0) {
            // diff returns 0 when files match
            printf
                ("  \033[32mTest PASSED: Output matches expected result.\033[0m\n\n");
            passedCount++;
        } else {
            // diff returns a non-zero code when files differ
            printf
                ("\033[0;31m  Test FAILED: Output differs from expected result.\033[0m\n\n");
        }
    }

    // Print a summary of how many tests passed
    printf ("Summary: %d out of %d tests passed.\n", passedCount, numTests);

    return 0;
}
