#include "pager.hpp"

void Pager::setVisible() {
    // Return a range of iterators for the boxes on the current page, or all boxes if there are no pages.
    size_t pageCount = indices.size();
    visible[0] = begin(boxes) + (pageCount ? *pageIt : 0);
    visible[1] = pageCount > 1 && pageIt != end(indices) - 1 ? begin(boxes) + *(pageIt + 1) : end(boxes);
}

void Pager::setBounds() {
    // Set bounds to enclose all boxes.
    size_t boxCount = boxes.size();
    if (!boxCount) return;
    // Get rects for all boxes.
    std::vector<SDL_Rect> rts;
    rts.reserve(boxes.size());
    std::transform(begin(boxes), end(boxes), std::back_inserter(rts), [](const std::unique_ptr<TextBox> &bx) { return bx->getRect(); });
    // Get minimum and maximum x and y for boxes.
    int xMin = rts[0].x, xMax = rts[0].x, yMin = rts[0].y, yMax = rts[0].y;
    for (auto &rt : rts) {
        xMin = std::min(xMin, rt.x);
        yMin = std::min(yMin, rt.y);
        xMax = std::max(xMax, rt.x + rt.w);
        yMax = std::max(yMax, rt.y + rt.h);
    }
    bounds = {xMin, yMin, xMax - xMin, yMax - yMin};
}

TextBox *Pager::getVisible(size_t idx) {
    // Get pointer to box with given index currently visible on this pager.
    return (visible[0] + idx)->get();
}

std::vector<TextBox *> Pager::getVisible() {
    // Get vector of pointers to all boxes currently visible on this pager.
    std::vector<TextBox *> bxs;
    std::transform(visible[0], visible[1], std::back_inserter(bxs), [](std::unique_ptr<TextBox> &bx) { return bx.get(); });
    return bxs;
}

std::vector<TextBox *> Pager::getAll() {
    std::vector<TextBox *> bxs;
    std::transform(begin(boxes), end(boxes), std::back_inserter(bxs),
                   [](std::unique_ptr<TextBox> &bx) { return bx.get(); });
    return bxs;
}

void Pager::addBox(std::unique_ptr<TextBox> &&bx) {
    if (indices.empty() || pageIt == end(indices) - 1)
        // There are no pages or we are on last page, put box on end of boxes vector.
        boxes.push_back(std::move(bx));
    else {
        // Add box to end of current page.
        auto nextPage = pageIt + 1;
        boxes.insert(begin(boxes) + *nextPage, std::move(bx));
        // Increase indices of all following pages.
        for (; nextPage != end(indices); ++nextPage) ++*nextPage;
    }
    setVisible();
    setBounds();
}

void Pager::addPage(std::vector<std::unique_ptr<TextBox>> &bxs) {
    // Move parameter boxes and pagers into member vectors giving them a new page. Sets page to first page.
    size_t boxCount = boxes.size();
    indices.push_back(boxCount);
    pageIt = begin(indices);
    boxCount += bxs.size();
    boxes.reserve(boxCount);
    std::move(begin(bxs), end(bxs), std::back_inserter(boxes));
    bxs.clear();
    setVisible();
    setBounds();
}

void Pager::reset() {
    boxes.clear();
    indices.clear();
    setVisible();
}

void Pager::recedePage() {
    // Recede to previous page. Prevent receding past first page.
    if (pageIt != begin(indices)) --pageIt;
    setVisible();
}

void Pager::advancePage() {
    // Advance to next page. Prevent advancing past last page.
    if (pageIt != end(indices) - 1) ++pageIt;
    setVisible();
}

void Pager::setPage(size_t pg) {
    pageIt = begin(indices);
    setVisible();
}

int Pager::getKeyedIndex(const SDL_KeyboardEvent &k) {
    // Return the index relative to current page of keyed box, or -1 if no box was keyed on current page.
    auto keyedIt =
        std::find_if(visible[0], visible[1], [&k](const std::unique_ptr<TextBox> &bx) { return bx->keyCaptured(k); });
    return keyedIt == visible[1] ? -1 : keyedIt - visible[0];
}

int Pager::getClickedIndex(const SDL_MouseButtonEvent &b) {
    // Return the index relative to current page of clicked box, or -1 if no box was clicked on current page.
    auto clickedIt =
        std::find_if(visible[0], visible[1], [&b](const std::unique_ptr<TextBox> &bx) { return bx->clickCaptured(b); });
    return clickedIt == visible[1] ? -1 : clickedIt - visible[0];
}

void Pager::draw(SDL_Renderer *s) {
    // Draw all boxes on the current page, or all boxes if there are no pages.
    for (auto bxIt = visible[0]; bxIt != visible[1]; ++bxIt) (*bxIt)->draw(s);
}
