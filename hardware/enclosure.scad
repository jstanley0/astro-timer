// NOTE: this box fits the version of the PCB in revision 675ee8522adee054eeff6e4886fde3a02e86f080
// the switch positions have been tweaked since then; see comments if building a box for a newly fabbed PCB

pcb_width=65.9;
pcb_length=41.4;

pcb_margin=0.5;

pcb_level=13.5;
pcb_thickness=1.6;
control_thickness=6.2;

inside_width=pcb_width+pcb_margin*2;
inside_length=pcb_length+pcb_margin*2;

inside_height=pcb_level+pcb_thickness+control_thickness;

wall_thickness=2;
floor_thickness=1.5;
overlap_thickness=5;
overlap_delta=0.25;

screw_sep_x=59.13;
screw_sep_y=34.8;

screw_int_r=2.5;
screw_ext_r=4.8;
screw_depth=8;
screw_ledge_depth=0.5;
screw_ledge_r=3;

standoff_int_r=1.5;
standoff_ext_r=2.5;
standoff_overlap=1.5;
standoff_support_size=standoff_ext_r*3;

screw_spacer_int_r=2;
standoff_spacer_int_r=1.75;
spacer_ext_r=3;

// note: offsets are relative to the bottom and right
    
screen_offset_x=12.84;
screen_offset_y=27.13;
screen_width=46;
screen_height=12.88;
screen_margin=0.5;
screen_support_width=2;
screen_support_thickness=3;

sw23_offset_y=5.64; // 5.74 in newest PCB design
sw3_offset_x=33.31; // 33.29
sw2_offset_x=42.36; // 41.98
sw_radius=1.95;

enc_offset_y=17.55;
enc_offset_x=8.78;
enc_radius=3.5;
enc_dep_radius=5.75;
enc_dep_thickness=0.5;

enc_nub_width=2.5;
enc_nub_height=1.5;
enc_nub_offset_y=11.25;
enc_nub_offset_x=enc_offset_x;

enc_body_width=12;
enc_body_height=14;

jack_offset_y=30.97;
jack_radius=4.8;
jack_height=2.65;

jack_body_offset_y=28;
jack_body_size_x=11.5;
jack_body_size_y=6;
jack_body_size_z=5.5;

isp_offset_x=48.28; // 49.04 in newest PCB design
isp_offset_y=1.41;  //  1.32 
isp_width=7.85;
isp_height=5.32;


module screw(rotation)
{
    rotate([0, 0, rotation])
    difference()
    {
        union() {
            cylinder(pcb_level, screw_ext_r, screw_ext_r, $fn=32);
            translate([-screw_ext_r, -screw_ext_r, 0])
                cube([screw_ext_r, screw_ext_r, pcb_level]);
        }
        translate([0, 0, pcb_level - screw_depth - screw_ledge_depth])
            cylinder(screw_depth, screw_int_r, screw_int_r, $fn=24);
        translate([0, 0, pcb_level - screw_ledge_depth])
            cylinder(screw_ledge_depth, screw_ledge_r, screw_ledge_r, $fn=24);
    }
}

module standoff(rotation)
{
    rotate([0, 0, rotation])
    union()
    {
        cylinder(pcb_level, standoff_ext_r, standoff_ext_r, $fn=32);
        translate([0, 0, pcb_level])
            sphere(standoff_int_r, $fn=16);
        translate([-2.33*standoff_ext_r, -2.33*standoff_ext_r, 0])
            cube([standoff_support_size, standoff_support_size, pcb_level]);
    }
}

module box()
{
    // coordinate space puts (0, 0, 0) just inside the box
    difference()
    {
        translate([-wall_thickness, -wall_thickness, -floor_thickness])
        {
            cube([inside_width + wall_thickness*2,
                  inside_length + wall_thickness*2,
                  inside_height + floor_thickness]);
        }
        cube([inside_width, inside_length, inside_height]);

        // jack
        translate([inside_width, jack_offset_y, pcb_level + pcb_thickness + jack_height])
            rotate([0, 90, 0])
            cylinder(wall_thickness, jack_radius, jack_radius, $fn=28);
        translate([inside_width + wall_thickness / 2, jack_offset_y, pcb_level + pcb_thickness + jack_height + jack_radius/2])
            cube([wall_thickness, jack_radius*2, jack_radius], true);
    }
    translate([inside_width/2 - screw_sep_x/2, inside_length/2 - screw_sep_y/2, 0])
        standoff(0);
    translate([inside_width/2 - screw_sep_x/2, inside_length/2 + screw_sep_y/2, 0])
        screw(270);
    translate([inside_width/2 + screw_sep_x/2, inside_length/2 - screw_sep_y/2, 0])
        screw(90);
    translate([inside_width/2 + screw_sep_x/2, inside_length/2 + screw_sep_y/2, 0])
        standoff(180);   
    
}

module lid()
{
    difference()
    {
        union()
        {
            // top
            translate([-wall_thickness, -wall_thickness, -floor_thickness])
            {
                cube([inside_width + wall_thickness*2,
                      inside_length + wall_thickness*2,
                      floor_thickness]);
            }
            // lip
            translate([overlap_delta, overlap_delta, 0])
            {
                difference()
                {
                    cube([inside_width - overlap_delta*2,
                          inside_length - overlap_delta*2,
                          overlap_thickness]);
                    translate([wall_thickness, wall_thickness, 0])
                    {
                        cube([inside_width - overlap_delta*2 - wall_thickness*2,
                              inside_length - overlap_delta*2 - wall_thickness*2,
                              overlap_thickness]);
                    }
                }
            }
            // spacer ext
            h = inside_height - pcb_level - pcb_thickness;
            translate([inside_width/2 - screw_sep_x/2, inside_length/2 - screw_sep_y/2, 0])
                cylinder(h, spacer_ext_r, spacer_ext_r, $fn=32);
            translate([inside_width/2 - screw_sep_x/2, inside_length/2 + screw_sep_y/2, 0])
                cylinder(h, spacer_ext_r, spacer_ext_r, $fn=32);
            translate([inside_width/2 + screw_sep_x/2, inside_length/2 - screw_sep_y/2, 0])
                cylinder(h, spacer_ext_r, spacer_ext_r, $fn=32);
            translate([inside_width/2 + screw_sep_x/2, inside_length/2 + screw_sep_y/2, 0])
                cylinder(h, spacer_ext_r, spacer_ext_r, $fn=32);
            
            // screen support
            translate([pcb_margin+screen_offset_x-screen_margin-screen_support_width,
                       pcb_margin+screen_offset_y-screen_margin-screen_support_width,
                       0])
                cube([screen_width+screen_support_width*2+screen_margin*2, screen_height+screen_support_width+screen_margin*2, screen_support_thickness]);
            
        }
        union()
        {
            // spacer int
            h = floor_thickness + inside_height - pcb_level - pcb_thickness;
            translate([inside_width/2 - screw_sep_x/2, inside_length/2 - screw_sep_y/2, -floor_thickness])
                cylinder(h, screw_spacer_int_r, screw_spacer_int_r, $fn=24);
            translate([inside_width/2 - screw_sep_x/2, inside_length/2 + screw_sep_y/2, 0])
                cylinder(h, standoff_spacer_int_r, standoff_spacer_int_r, $fn=24);
            translate([inside_width/2 + screw_sep_x/2, inside_length/2 - screw_sep_y/2, 0])
                cylinder(h, standoff_spacer_int_r, standoff_spacer_int_r, $fn=24);
            translate([inside_width/2 + screw_sep_x/2, inside_length/2 + screw_sep_y/2, -floor_thickness])
                cylinder(h, screw_spacer_int_r, screw_spacer_int_r, $fn=24);
            
            // screen window
            translate([pcb_margin + screen_offset_x - screen_margin,
                       pcb_margin + screen_offset_y - screen_margin,
                       -floor_thickness])
                cube([screen_width + screen_margin*2, screen_height + screen_margin*2, floor_thickness + overlap_thickness]);
                            
            // sw2 hole
            translate([pcb_margin + sw2_offset_x,
                       pcb_margin + sw23_offset_y,
                       -floor_thickness])
               cylinder(floor_thickness, sw_radius, sw_radius, $fn=16);
            
            
            // sw3 hole
            translate([pcb_margin + sw3_offset_x,
                       pcb_margin + sw23_offset_y,
                       -floor_thickness])
               cylinder(floor_thickness, sw_radius, sw_radius, $fn=16);            
            
            // enc hole
            translate([pcb_margin + enc_offset_x,
                       pcb_margin + enc_offset_y,
                       -floor_thickness])
               cylinder(floor_thickness, enc_radius, enc_radius, $fn=28);                 
            
            // enc dep
            translate([pcb_margin + enc_offset_x,
                       pcb_margin + enc_offset_y,
                       -enc_dep_thickness])
               cylinder(floor_thickness, enc_dep_radius, enc_dep_radius, $fn=36);

            // enc body (actually I believe this has no effect)
            translate([pcb_margin + enc_offset_x - enc_body_width/2,
                       pcb_margin + enc_offset_y - enc_body_height/2,
                       0])
                cube([enc_body_width, enc_body_height, overlap_thickness]);
            
            // enc nub
            translate([pcb_margin + enc_nub_offset_x,
                       pcb_margin + enc_nub_offset_y,
                       -floor_thickness/2])
               cube([enc_nub_width, enc_nub_height, floor_thickness], true);                             
            
            // jack
            translate([-wall_thickness, jack_offset_y, control_thickness - jack_height])
                rotate([0, 90, 0])
                cylinder(wall_thickness*2+overlap_delta, jack_radius, jack_radius, $fn=28);
            translate([wall_thickness/2 + overlap_delta, jack_offset_y, control_thickness - jack_height + jack_radius/2])
                cube([wall_thickness, jack_radius*2, jack_radius], true);                
                
            // jack body
            translate([wall_thickness + overlap_delta,
                       jack_body_offset_y,
                       control_thickness - jack_body_size_z])
               cube([jack_body_size_x, jack_body_size_y, jack_body_size_z]);
                       
            
            // isp
            //translate([pcb_margin + isp_offset_x,
            //           pcb_margin + isp_offset_y,
            //           -floor_thickness])
            //   cube([isp_width, isp_height, floor_thickness]);
            
        }
    }
}

box();

translate([0, inside_length + 4 * wall_thickness, 0])
    lid();
