#include <lcadengine/logicTable.h>
#include <stdlib.h>

#define GLI genericLogicInterface

struct s_context {
	hashmap *GIDMap;
	hashmap *CIDMap, *srcMap, *drnMap;
};

context *create_context() {
    context *ctx = (context*) malloc(sizeof(context));
    ctx->GIDMap = hashmapCreate(0);
    ctx->CIDMap = hashmapCreate(0);
    ctx->srcMap = hashmapCreate(0);
    ctx->drnMap = hashmapCreate(0);
    return ctx;
};

void delete_context(context *ctx) {
    hashmapDelete(ctx->GIDMap);
    hashmapDelete(ctx->CIDMap);
    hashmapDelete(ctx->srcMap);
    hashmapDelete(ctx->drnMap);
    free(ctx);
};

uint64_t add_gli(context *ctx, gateInputType type, bool nin, uint8_t delay) {
    GLI *gli = (GLI*) malloc(sizeof(GLI));
    
    gli->ID = (uint64_t) gli // We're using the pointer as a UUID, as we don't have a generator.
    // TODO: Generate Better IDs
    gli->state = false; // All GLI's start off
    // Just copy this across.
    gli->delay = delay;
    gli->inputMode = type;
    gli->inputNegate = nin;
    
    // Push gli into the map
    hashmapInsert(ctx->GIDMap, (void*)gli, gli->ID);
    return gli->ID;
    // gli goes out of scope here. (psst, don't tell anyone the ID is a pointer)
}

void remove_gli(context *ctx, uint64_t ID) {
     GLI *gli = (GLI*) hashmapRemove(ctx->GIDMap, ID);
     //TODO Remove connections here.
     free(gli);
}
