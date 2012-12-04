pcb_w = 56;
pcb_l = 93;

coil_w = 2;
coil_h = 33;
coil_l = 43;

rfid_l = 42;
rfid_w = 24;

module rounded_cube(sx,sy,sz,r) 
{
	union()
	{
		translate([r,r,0]) cube([sx-2*r,sy-2*r,sz],false);
		translate([r,0,r]) cube([sx-2*r,sy,sz-2*r],false);
		translate([0,r,r]) cube([sx,sy-2*r,sz-2*r],false);

		translate([r,r,r]) rotate(a=[0,90,0]) cylinder(h=sx-2*r,r=r,center=false);
		translate([r,sy-r,r]) rotate(a=[0,90,0]) cylinder(h=sx-2*r,r=r,center=false);
		translate([r,r,sz-r]) rotate(a=[0,90,0]) cylinder(h=sx-2*r,r=r,center=false);
		translate([r,sy-r,sz-r]) rotate(a=[0,90,0]) cylinder(h=sx-2*r,r=r,center=false);

		translate([r,r,r]) rotate(a=[0,0,0]) cylinder(h=sz-2*r,r=r,center=false);
		translate([r,sy-r,r]) rotate(a=[0,0,0]) cylinder(h=sz-2*r,r=r,center=false);
		translate([sx-r,r,r]) rotate(a=[0,0,0]) cylinder(h=sz-2*r,r=r,center=false);
		translate([sx-r,sy-r,r]) rotate(a=[0,0,0]) cylinder(h=sz-2*r,r=r,center=false);

		translate([r,r,r]) rotate(a=[-90,0,0]) cylinder(h=sy-2*r,r=r,center=false);
		translate([r,r,sz-r]) rotate(a=[-90,0,0]) cylinder(h=sy-2*r,r=r,center=false);
		translate([sx-r,r,r]) rotate(a=[-90,0,0]) cylinder(h=sy-2*r,r=r,center=false);
		translate([sx-r,r,sz-r]) rotate(a=[-90,0,0]) cylinder(h=sy-2*r,r=r,center=false);

		translate([r,r,r]) sphere(r);
		translate([r,sy-r,r]) sphere(r);
		translate([r,r,sz-r]) sphere(r);
		translate([r,sy-r,sz-r]) sphere(r);

		translate([sx-r,r,r]) sphere(r);
		translate([sx-r,sy-r,r]) sphere(r);
		translate([sx-r,r,sz-r]) sphere(r);
		translate([sx-r,sy-r,sz-r]) sphere(r);
	}
}

module base() {
  rounded_cube(pcb_l+25, pcb_w+5, pcb_w+5, 8);
}


module cover(r) {
  translate([0, 15/2, pcb_w])
  cube([pcb_l, pcb_w - 10, 5]);
}

module insides() {
  cube([coil_w, coil_l, coil_h]);
  cube([rfid_l, rfid_w, 5]);
}

cover();