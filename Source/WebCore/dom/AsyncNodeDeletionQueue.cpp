/*
 * Copyright (C) 2024 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"
#include "AsyncNodeDeletionQueue.h"

#include "HTMLElement.h"
#include "NodeName.h"

#define MAX_SIZE_ASYNC_NODE_DELETION_QUEUE 500'000

namespace WebCore {

static bool isNodeLikelySmall(const Node& node)
{
    /*
    static WTF::NeverDestroyed<std::unordered_map<ElementName, unsigned>> counters;
    ASSERT(node.isElementNode());
    auto& element = downcast<Element>(node);
    auto elementName = element.elementName();
    auto find = counters->find(elementName);
    if (find == std::end(counters.get()))
        counters.get()[elementName] = 1;
    else
        counters.get()[elementName] += 1;
    WTF_ALWAYS_LOG("element " << element.tagName() << " counter: " << counters.get()[elementName]);
    */

    switch (downcast<Element>(node).elementName()) {
    case NodeName::HTML_input:
    case NodeName::HTML_li:
    case NodeName::HTML_a:
    case NodeName::HTML_div:
    case NodeName::HTML_button:
    case NodeName::HTML_ul:
    case NodeName::HTML_label:
    case NodeName::HTML_link:
    case NodeName::HTML_p:
    case NodeName::HTML_select:
    case NodeName::HTML_form:
    case NodeName::HTML_code:
    case NodeName::HTML_i:
    case NodeName::HTML_listing:
    case NodeName::HTML_hr:
    case NodeName::HTML_summary:
    case NodeName::HTML_details:
    case NodeName::HTML_nav:
    case NodeName::HTML_h1:
    case NodeName::HTML_h2:
    case NodeName::HTML_h3:
    case NodeName::HTML_h4:
    case NodeName::HTML_h5:
    case NodeName::HTML_h6:
    case NodeName::HTML_em:
    case NodeName::HTML_strong:
    case NodeName::HTML_ol:
    case NodeName::HTML_address:
    case NodeName::HTML_b:
    case NodeName::HTML_span:
    case NodeName::HTML_abbr:
    case NodeName::HTML_q:
    case NodeName::HTML_option:
    case NodeName::HTML_cite:
    case NodeName::HTML_mark:
    case NodeName::HTML_article:
        return true;
    default:
        return false;
    }
}

void AsyncNodeDeletionQueue::add(NodeVector&& children, unsigned numberOfNodes)
{
    if (m_numberOfNodes + numberOfNodes > MAX_SIZE_ASYNC_NODE_DELETION_QUEUE) {
        // Synchronously deleted nodes.
        children.clear();
        return;
    }

    m_queue.appendVector(WTFMove(children));
    m_numberOfNodes += numberOfNodes;

    //WTF_ALWAYS_LOG("queue size: " << m_numberOfNodes);
}

bool AsyncNodeDeletionQueue::canNodeBeAsyncDeleted(const Node& node)
{
    if (!dynamicDowncast<HTMLElement>(node))
        return true;
    if (!isNodeLikelySmall(node))
        return false;
    return true;
}

void AsyncNodeDeletionQueue::clear()
{
    // WTF_ALWAYS_LOG("clearing queue from size " << m_queue.size());
    m_queue.clear();
    m_numberOfNodes = 0;
}

} // namespace WebCore
