#ifndef RENDER_BUFFER
#define RENDER_BUFFER

#include "PxrUsage.h"

PXR_NAMESPACE_USING_DIRECTIVE

class VkImage;
class VkImageView;

class ExRenderBuffer final : public HdRenderBuffer
{
public:

    /// Get allocation information from the scene delegate.
    /// Note: Embree overrides this only to stop the render thread before
    /// potential re-allocation.
    ///   \param sceneDelegate The scene delegate backing this render buffer.
    ///   \param renderParam   The renderer-global render param.
    ///   \param dirtyBits     The invalidation state for this render buffer.
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam *renderParam,
              HdDirtyBits *dirtyBits) override;

    /// Deallocate before deletion.
    ///   \param renderParam   The renderer-global render param.
    /// Note: Embree overrides this only to stop the render thread before
    /// potential deallocation.
    void Finalize(HdRenderParam *renderParam) override;

    /// Allocate a new buffer with the given dimensions and format.
    ///   \param dimensions   Width, height, and depth of the desired buffer.
    ///                       (Only depth==1 is supported).
    ///   \param format       The format of the desired buffer, taken from the
    ///                       HdFormat enum.
    ///   \param multisampled Whether the buffer is multisampled.
    ///   \return             True if the buffer was successfully allocated,
    ///                       false with a warning otherwise.
    bool Allocate(GfVec3i const& dimensions,
                  HdFormat format,
                  bool multiSampled) override;

    /// Accessor for buffer width.
    ///   \return The width of the currently allocated buffer.
    unsigned int GetWidth() const override;

    /// Accessor for buffer height.
    ///   \return The height of the currently allocated buffer.
    unsigned int GetHeight() const override;

    /// Accessor for buffer depth.
    ///   \return The depth of the currently allocated buffer.
    unsigned int GetDepth() const override;

    /// Accessor for buffer format.
    ///   \return The format of the currently allocated buffer.
    HdFormat GetFormat() const override;

    /// Accessor for the buffer multisample state.
    ///   \return Whether the buffer is multisampled or not.
    bool IsMultiSampled() const override;

    /// Map the buffer for reading/writing. The control flow should be Map(),
    /// before any I/O, followed by memory access, followed by Unmap() when
    /// done.
    ///   \return The address of the buffer.
    void* Map() override;

    /// Unmap the buffer.
    void Unmap() override;

    /// Return whether any clients have this buffer mapped currently.
    ///   \return True if the buffer is currently mapped by someone.
    bool IsMapped() const override;

    /// Is the buffer converged?
    ///   \return True if the buffer is converged (not currently being
    ///           rendered to).
    bool IsConverged() const override;

    /// Set the convergence.
    ///   \param cv Whether the buffer should be marked converged or not.
    void SetConverged(bool cv);

    /// Resolve the sample buffer into final values.
    void Resolve() override;

private:

    // Release any allocated resources.
    void _Deallocate() override;
};

#endif