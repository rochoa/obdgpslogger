/** \file
 \brief Generate data from FLTK GUI
*/

/* Do not bother trying to compile this. It's just going to be
    #included in another C file. It's here to work around a limitation
    in fluid. */

struct obdsim_generator obdsimgen_gui_fltk = {
	guifltk_simgen_name,
	guifltk_simgen_longdesc,
	guifltk_simgen_create,
	guifltk_simgen_destroy,
	guifltk_simgen_getvalue,
	guifltk_simgen_idle
};

