#include <lcadengine/dispatcher.h>

#include <stdlib.h>
#include <string.h>

#include "utils/thpool.h"
#include "utils/fastlist.h"

#ifndef MAX_DELAY
    #define MAX_DELAY 100
#endif

#define GLI genericLogicInterface

struct {
    GLI* unit;
    long timestep;
    dispatcher *ctx;
} typedef job;

struct {
    unsigned long ID;
    bool newState;
} typedef diff;

struct s_dispatcher {
    threadpool pool;
    unsigned long timestep, n;
    graph *LG;
    // Array containing all jobs for all upcoming timesteps.
    job **jobpool;
    // Array containing number of used slots in each given timestep.
    unsigned long *jobpoolCount; // size = (MAX_DELAY + 1)
    diff *diffBuffer;
    unsigned long diffBufferCount;
};

inline void generate_job(dispatcher *ctx, GLI *unit, unsigned int offset);
void worker_do_work(job *j);

dispatcher *create_dispatcher(graph *logicGraph, int threads) {
    dispatcher* ctx = (dispatcher*) malloc(sizeof(dispatcher));
    ctx->LG = logicGraph;
    //TODO: Lock Graph for editing;
    ctx->timestep = 0;
    ctx->pool = thpool_init(threads);
    ctx->n = get_node_count(logicGraph);
    // make a big thing to store Jobs in.
    ctx->jobpool = (job**) malloc(ctx->n * (MAX_DELAY + 1) * sizeof(job));
    memset(ctx->jobpool, 0, ctx->n * (MAX_DELAY + 1) * sizeof(job)); 
    // Make a list of jobs in each timestep.
    ctx->jobpoolCount = (unsigned long*) malloc((MAX_DELAY + 1) * sizeof(unsigned long));
    memset(ctx->jobpoolCount, 0, (MAX_DELAY + 1) * sizeof(unsigned long));
    // make a buffer to store the differance in the graph from a timestep.
    ctx->diffBuffer = (diff*) malloc(ctx->n * sizeof(job));
    memset(ctx->diffBuffer, 0, ctx->n * sizeof(job));
    ctx->diffBufferCount = 0;
}

void delete_dispatcher(dispatcher *ctx) {
    thpool_destroy(ctx->pool);
    free(ctx->jobpool);
    free(ctx->jobpoolCount);
    free(ctx->diffBuffer);
    free(ctx);
}

int step_dispatcher(dispatcher *ctx) {
    // Move context into the next step.
    ctx->timestep++;
    
    // Loopy Stuff
    unsigned long i;
    
    // Constanty stuff
    unsigned long time = ctx->timestep % (MAX_DELAY + 1);
    
    // Adds each job to the threadpool's queue for execution.
    for (i = 0; i < ctx->jobpoolCount[time]; i++) {
        job *j = &ctx->jobpool[time][i];
        thpool_add_work(ctx->pool, (void*) worker_do_work, (void*) j);
    }
    
    // Wait for the step execution to complete.
    thpool_wait(ctx->pool);
    
    // here the diff buffer is populated. 
    
    // apply the diff pactches to the graph. 
    for (i = 0; i < ctx->diffBufferCount; i++) {
        diff *d = &ctx->diffBuffer[i];
        GLI *g = get_gli(ctx->LG, d->ID);
        g->state = d->newState;
    }
    
    // In both of these memset commands only the used memory is cleared, 
    // so as to not to waste time clearing blank memory. 
    
    // Clear the Job pool for this step;
    memset(ctx->jobpool[time], 0, ctx->jobpoolCount[time] * sizeof(job));
    ctx->jobpoolCount[time] = 0;
    
    // Clear the diffBuffer now that the patches are applied.
    memset(ctx->diffBuffer, 0, ctx->diffBufferCount * sizeof(diff));
    ctx->diffBufferCount = 0;
    return 0;
}

void worker_do_work(job *j) {
    // Get Inputs;
    fastlist *inputs = get_conns_by_drn(j->ctx->LG, j->unit->ID);  
    unsigned long count = fastlist_size(inputs);
    unsigned long i;
    unsigned long sum = 0;
    for (i = 0; i < count; i++) {
        // get The source gate for the connection.
        connection *conn = (connection*) fastlist_get(inputs, i);
        GLI *in = conn->srcEp;
        // add the gate to the input sum.
        sum += in->state;
    }
    
    // Compare state;
    bool output = false;
    switch (j->unit->inputMode) {
        case AND:   if (sum == count) output = true; break; 
        case UNITY: // Behaves like a 1 input OR
        case OR:    if (sum > 0)      output = true; break;
        case XOR:   if (sum == 1)     output = true; break;
        case RAND: 
        default: break;
    }
    if (j->unit->inputNegate) output != output;
    
    // If state has changed:
    if (output != j->unit->state) {
        // Register change with diff buffer;
        j->ctx->diffBuffer[j->ctx->diffBufferCount++].ID = j->unit->ID;
        j->ctx->diffBuffer[j->ctx->diffBufferCount++].newState = output;
        
        // Get Outputs;
        fastlist *outputs = get_conns_by_src(j->ctx->LG, j->unit->ID);
        count = fastlist_size(outputs);
        for (i = 0; i < count; i++) {
            // get The source gate for the connection.
            connection *conn = (connection*) fastlist_get(outputs, i);
            GLI *out = conn->drnEp;
            // Generate Job;
            generate_job(j->ctx, out, 1); // TODO: include delay.
        }
    }
}

inline void generate_job(dispatcher *ctx, GLI *unit, unsigned int offset) {
    if (offset > MAX_DELAY) {
        //TODO: Error.
    }
    unsigned long time = ctx->timestep + offset;
    unsigned long i = time % (MAX_DELAY + 1);
    unsigned long j = ctx->jobpoolCount[i]++;
    ctx->jobpool[i][j].unit = unit;
    ctx->jobpool[i][j].ctx = ctx;
    ctx->jobpool[i][j].timestep = time;
}

