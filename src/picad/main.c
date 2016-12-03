#include "IO.h"
#include "fileparser.h"

#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>

#include "../lcadengine/base/logicGraph.h"
#include "../lcadengine/sim/dispatcher.h"

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
    signal(SIGINT, intHandler);
}

int useconds(void) {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return 1000000 * tv.tv_sec + tv.tv_usec;
}

int main(int argc, char *argv[]) {
    // Set Up GPIO
    wiringPiSetup();
	// Read File
	struct fileInfoDataset file = parseFile(argv[1]);
    
    // TODO: Get this.
    // speed of simulation (in µs)
    int delay = 1000; //(1ms)

	// Load engine
	dispatcher *d = dispatcherCreate(file.g, 0);
        
    // Run until someone presses ^C
    signal(SIGINT, intHandler);

    for(int j = 0; j < graphGetNodeCount(file.g); j++) {
    	dispatcherAddJob(d, j, 1);
    }

    for(int j = 0; j < 100; j++) {
    	dispatcherStep(d);
    }

	while(keepRunning) {
        //get time 
        int start = useconds();
        // Read inputs
        for (size_t i = 0; i < file.inputCount; i++) {
            bool in = readInput(file.inputs[i]);
            graphGetGLI(file.g, file.inputs[i]->ID)->state = in;
	        dispatcherAddJob(d, file.inputs[i]->ID, 1);
        }

		// Run simulation one step
		dispatcherStep(d);

		// Update outputs
		for (size_t i = 0; i < file.outputCount; i++) {
            writeOutput(file.outputs[i], graphGetGLI(file.g, file.inputs[i]->ID)->state);
        }
        
        //get time again
        int end = useconds();
        int time = end - start;
        if (time > delay) {
            printf("Can't Keep Up. Delay set to %iµs, update took %iµs\n\n", delay, time);
        } else {
            usleep((delay - time));
        }        
	}

    // Cleanup
	dispatcherDelete(d);
	
    // Cleanup GPIO 
    cleanFile(file);
 
    return 0;   
    
	// kill all humans

	return 0;
}
