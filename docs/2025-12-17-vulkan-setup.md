# vulkan-setup

- date: 2025-12-17
- milestone: triangle drawn on screen

## These notes 

These notes, if one could call them that, serve as posterity to decisions made therein and henceforth to the engine. 
These notes are informal and likely to be wholly egregious to those of eyes and sound mind--and, of course, the ability to read.

These notes shall be construct in a skimmable way, if skimmable is not a word then I am saddened, such that one can read quickly
through them if need be or read the in-between text for those so inclined to listen to the zounds and triumpth of the desolate
hummer of this codebase.

The vulkan setup is conducted in four parts as of now. Those being:

- the core 
- the frame 
- the presentation 
- and the render
- as well as the shaders.

This separation seems logical as it groups those of whom that are born together
and ought, by the hypostasis of the world, die together.

I shall also append at the end a quick-to-read list of that which likely needs changing in the future.

Now, the codebase parts are Derridan instead of a more platonic,
hierarchical sense, though their genesis and eschatology is stacked linearly.

## The four parts broken down

### Core

The core take care of the genesis and that which calls them culls them, that being the render/renderer.

This part also takes care of the vulkan validation layer, and its messenger, The Unsanctimonious itself.

### Presentation 

The presentation layer handles both the swapchain and the surface upon which we draw.

The swapchain is initialized with a triparte suite. This being:

- The Surface Format
- The Present Mode
- and The extent

Now, as per usual the surface format is chosen such that BGRA8 and SRGB_NONLINEAR is preferred,
if one cannot have such luxuries the first format is choosen.

The present mode, if it has the axiom of choice, shall choose MailBox, then FIFO, if on MailBox,
then, solemnly, fallback to the first of kin.

The extent, which is just the resolution of the surface, why is it called extent, is choosen
for simply taking the widht and height of the surface.

Now the swapchain's eschatology shall be discussed.

The swapchain must be rebuilt when the window size has changed,
or the window placement has changed. To aid, vkAcquirenextImageKHR
and vkPresentNextImageKHR shall return flags of types in another triparte:

- VK_ERROR_OUT_OF_DATE_KHR
- VK_SUBOPTIMAL_KHR
- VK_SUCCESS

Upon OUT_OF_DATE, one shall weep for the fall of the swapchain and ask
permission for the rebuilding of it for this frame is with this swapchain
is unpresentable.

Now, the engine of the laity is built upon the premise that
SUBOPTIMAL_KHR is the same as success, and this engine also falls
into this hedonism, for brevity instead of sin.

This is one of the aforementioned things which need changing.
For the proposal for this change see Appendix #1.

### Render 

Here lives the logical structures of the render pass, pipeline,
and framebuffers.

See Appendix 2 for comments on the future of pipeline.

In this, many hardcoded states breathe on the unsung words 
of the instantiated triangle. Once we move towards arbitrary 3d 
meshes, this shall be rectified.

I have little to say about this since most of the topics here are straightforward.

The render pass handles a render when a pass starts and stops it, hopefully successfully,
when it ends.

The graphics pipeline is an initialization haven where much well known
graphical contents, such as the viewport, depth testing, MSAA, rasterization, 
and of course shader loading lives. An orchestrator of sorts.
And the frame buffer is very on the nose.

The render is the last true mouth piece and grand orchestrator of all aforementioned
in this Render part.

### frame

This is were the synchronization logic lives. A pair of semaphores
are created per frame and a single fence lives to sync with the CPU.
This, as I understand it, is a rather normal setup and it works fine.
We store two framesInFlight at a maximum so as to not have tearing.

### Philsophy

The paramount force in kantian metaphysics is that of disrupting
the platonic view of realms, seen in a demiurgical sense, material that is,
and as in platonic forms, see in The republic.

Kant dismissed the realm of Forms as the panacea in which the lowly human
may hope to ascend to upon a well maintained life and instead rendered it unknowable.
The noumenal realm he called this panacea and phenomenal the lowly life forms sprawled.


One must propose that the noumenal realm is dead.

The post kantian heterodoxy, specifically Hegel, dismissed the noumenal and 
spoke of the false hypostatizaion of the phenomenal realm and found great
solace in the true noumenal realm, that is The Idea.

If all langauge is languid, lazy fronts, and the human must solipsize,
then must it not be that hypostatization is solpsism. The noumenal world is dead
and we are left asunder.

### Conclusion

On that cheery clause, this note is probably detailed enough for the roughly 1700 LOC 
written. Why is it such a pain to write prose in nvim? Everyone knows
nvim is a word copy cat.


## Appendices

### Appendix #1

A scheduler shall be made such that upon SUBOPTIMAL_KHR
the swapchain shall be rebuilt upon first convenient time instead of 
instantly. This convenient time shall be chosen such that no frames in flight
are currently using the old swapchain and shall be revoked access, this is usually
saved with a vkWait but we want to avoid waiting much time altogether.

### Appendix #2 

The pipeline must be updated to support non hardcoded values
to handle arbitrary 3d meshes. Then depth testing, so for the GPU
to know in what order to render, shall be enabled. 

