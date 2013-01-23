/*
 * yuy2.h
 *
 *  Created on: Jan 23, 2013
 *      Author: yegor
 */

#ifndef YUY2_H_
#define YUY2_H_

namespace yuy2 {
	struct luma_t {};

// chroma-blue and chroma-red fields are not distinguished
	struct chroma_t {};

	typedef boost::mpl::vector2<luma_t,chroma_t> color_space_t;

	typedef boost::gil::layout<color_space_t> pixel_layout_t;

//    template <typename, typename>    struct pixel;
//    template <typename, typename>    struct planar_pixel_reference;
//    template <typename, typename>    struct planar_pixel_iterator;
//    template <typename>                class memory_based_step_iterator;
//    template <typename>                class point2;
//    template <typename>                class memory_based_2d_locator;
//    template <typename>                class image_view;
//    template <typename, bool, typename>    class image;
    typedef boost::gil::pixel<boost::gil::bits8, pixel_layout_t> pixel_t;
    typedef const boost::gil::pixel<boost::gil::bits8, pixel_layout_t> c_pixel_t;
    typedef boost::gil::pixel<boost::gil::bits8, pixel_layout_t>& ref_t;
    typedef const boost::gil::pixel<boost::gil::bits8, pixel_layout_t>& c_ref_t;
    typedef pixel_t* ptr_t;
    typedef c_pixel_t* c_ptr_t;
    typedef boost::gil::memory_based_step_iterator<ptr_t>                               step_ptr_t;
    typedef boost::gil::memory_based_step_iterator<c_ptr_t>                               c_step_ptr_t;
    typedef boost::gil::memory_based_2d_locator<boost::gil::memory_based_step_iterator<ptr_t> >       loc_t;
    typedef boost::gil::memory_based_2d_locator<boost::gil::memory_based_step_iterator<c_ptr_t> >       c_loc_t;
    typedef boost::gil::memory_based_2d_locator<boost::gil::memory_based_step_iterator<step_ptr_t> >  step_loc_t;
    typedef boost::gil::memory_based_2d_locator<boost::gil::memory_based_step_iterator<c_step_ptr_t> > c_step_loc_t;
    typedef boost::gil::image_view<loc_t>                                        view_t;
    typedef boost::gil::image_view<c_loc_t>                                        c_view_t;
    typedef boost::gil::image_view<step_loc_t>                                    step_view_t;
    typedef boost::gil::image_view<c_step_loc_t>                                   c_step_view_t;
    typedef boost::gil::image<pixel_t,false,std::allocator<unsigned char> >           image_t;


//	typedef boost::gil::image_type<boost::gil::bits8,pixel_layout_t> image_t;
//    GIL_DEFINE_BASE_TYPEDEFS
};




#endif /* YUY2_H_ */
