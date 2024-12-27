#pragma once

class RenderGraphNode;

struct RenderGraphOrder {
    RenderGraphNode* src;
    RenderGraphNode* dst;
};
