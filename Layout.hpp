#ifndef LAYOUT_HPP
#define LAYOUT_HPP

#include <QtCore/QSize>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <pthread.h>
#include <math.h>
#include <tuple>
#include <utility>

#include "Partitioner.hpp"

#define TreeDepthPixels 30
#define TreeWidthPixels 10

namespace LAYOUT
{
    void *initPartitioning(void *Args);

    // some of QT capabilities can only be inherited, I'm forced to use a class
    // here although nothing really requires an OOP structure.
    class LayoutWidget : public QWidget
    {
        protected:
            QPainterPath* Tree;
            uint32_t Width;
            std::pair<uint32_t, uint32_t> OriginalStartPoint;
            // Usually how this goes is that we draw everything in this paintEvent
            // which gets periodically called, and can be called by us. The problem
            // is that this thing does not do incremental changes, meaning it will
            // traverse the whole tree everytime we draw!! I don't want to be
            // that stupid, and that's why we use the QPainterPath above.
            void paintEvent(QPaintEvent *event);

        public:
            LayoutWidget(uint32_t ThreadCount, bool ShowNets, bool Verbose);
            std::pair<uint32_t, uint32_t> getOriginalStartPoint();
            std::pair<uint32_t, uint32_t> paintTreeBranch(std::pair<uint32_t, uint32_t> Start, uint32_t Level, bool Side, bool Verbose);
    };
}

#endif // LAYOUT_HPP
