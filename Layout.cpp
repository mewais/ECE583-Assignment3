#include "Layout.hpp"

namespace LAYOUT
{
    void *initPartitioning(void *Args)
    {
        // This is a stupid intermediate function needed for pthreads.
        uint32_t NumThreads = std::get<0>(*(std::tuple<uint32_t, LAYOUT::LayoutWidget *, bool>*)Args);
        LAYOUT::LayoutWidget *GUI = std::get<1>(*(std::tuple<uint32_t, LAYOUT::LayoutWidget *, bool>*)Args);
        bool Verbose = std::get<2>(*(std::tuple<uint32_t, LAYOUT::LayoutWidget *, bool>*)Args);
        if (Verbose)
            std::cout << "EPartitioner: Initializing partitioning.\n";
        PARTITIONER::setNumberOfThreads(NumThreads, Verbose);
        PARTITIONER::attachGUI(GUI);
        PARTITIONER::initPartitioning(Verbose);
        PARTITIONER::startPartitioning(Verbose);
        return NULL;                // disable stupid gcc warning
    }

    LayoutWidget::LayoutWidget(uint32_t ThreadCount, bool ShowNets, bool Verbose)
    {
        // Show the window and adjust the size to the size of layout
        // The height depends on the number of rows and channel spaces,
        // The width depends on the number of cols only.
        if (Verbose)
            std::cout << "EPartitioner: Starting GUI.\n";
        this->resize(std::pow(2, PARTITIONER::getNumberOfBlocks())*TreeWidthPixels,
                PARTITIONER::getNumberOfBlocks()*TreeDepthPixels);
        this->Width = std::pow(2, PARTITIONER::getNumberOfBlocks())*TreeWidthPixels;
        this->OriginalStartPoint = std::make_pair(std::pow(2, PARTITIONER::getNumberOfBlocks())*TreeWidthPixels/2,
                                        0);
        if(Verbose)
            std::cout << "EPartitioner: GUI: Tree Start Point at " << std::pow(2, PARTITIONER::getNumberOfBlocks())*TreeWidthPixels/2 << ", " << 0 << "\n";
        this->show();

        Tree = new QPainterPath();

        // Now we have initialized the GUI, we can now start the router
        pthread_t PartitionThread;
        std::tuple<uint32_t, LAYOUT::LayoutWidget *, bool>* Params;
        Params = new std::tuple<uint32_t, LAYOUT::LayoutWidget *, bool>;
        *Params = std::make_tuple(ThreadCount, this, Verbose);
        pthread_create(&PartitionThread, NULL, initPartitioning, (void *)(Params));
        pthread_join(PartitionThread, NULL);
    }

    std::pair<uint32_t, uint32_t> LayoutWidget::getOriginalStartPoint()
    {
        return this->OriginalStartPoint;
    }

    std::pair<uint32_t, uint32_t> LayoutWidget::paintTreeBranch(std::pair<uint32_t, uint32_t> Start, uint32_t Level, bool Side, bool Verbose)
    {
        // This thing incrementally draws tree branches, we change the width on each
        // level, making the higher branches wider than the lower branches.
        Tree->moveTo(std::get<0>(Start), std::get<1>(Start));
        if (Verbose)
            std::cout << "EPartitioner: GUI: Drawing Line from " << std::get<0>(Start) << ", " << std::get<1>(Start)
                    << " to ";
        if (Side) // go right
        {
            Tree->lineTo(std::get<0>(Start)+(this->Width/std::pow(2,Level+1)), std::get<1>(Start)+TreeDepthPixels);
            if (Verbose)
                std::cout << std::get<0>(Start)+(this->Width/std::pow(2,Level+1)) << ", " << std::get<1>(Start)+TreeDepthPixels << "\n";
            return std::make_pair(std::get<0>(Start)+(this->Width/std::pow(2,Level+1)), std::get<1>(Start)+TreeDepthPixels);
        }
        else // go left
        {
            Tree->lineTo(std::get<0>(Start)-(this->Width/std::pow(2,Level+1)), std::get<1>(Start)+TreeDepthPixels);
            if (Verbose)
                std::cout << std::get<0>(Start)-(this->Width/std::pow(2,Level+1)) << ", " << std::get<1>(Start)+TreeDepthPixels << "\n";
            return std::make_pair(std::get<0>(Start)-(this->Width/std::pow(2,Level+1)), std::get<1>(Start)+TreeDepthPixels);
        }
    }

    void LayoutWidget::paintEvent(QPaintEvent *event)
    {
        // Create a Paint Area and Painter
        QPainter GridPainter(this);
        GridPainter.setPen(Qt::black);
        GridPainter.drawPath(*Tree);
    }
}
