wall = 3;
l = 103;
h = 28 + wall;
w = 56;

box_w = w + wall;
box_l = l + 31;

nut_radius = 7.9/2;
nut_height = 3.4;

module nut() {
  translate([0, 0, -20])
  cylinder(h=30, r=2.2, $fn=50);
  cylinder(h=nut_height, r=nut_radius, $fn=6);
}


module pcb_cavity() {
  translate([0, 9, 0])
    cube([w, l - 9, h]);
  translate([20, 0, 3])
    cube([12, 9, 11]);
  translate([20, wall, 3])
    cube([12, 9 - wall, h - 3]);
}

module coil_cavity() {
  cube([45, 35, 2]);
}

module indicator_light_cavity() {
  rotate(v=[0, 1, 0], a=90) {
    cylinder(h = 35, r = 4.3, $fn = 50);
    translate([0, 0, 11.5])
    cylinder(h = 10, r = 5.5, $fn = 50);
  }
}

module button_cavity() {
  rotate( v = [0,1,0], a = 90 ) {
    cylinder(30, r = 3.7, $fn=50 );
  }
}

module battery_cavity() {
  translate([0, 0, 20])
  cube([10, 3, 8]);
  translate([0, 3, 0])
  cube([w, 20, h]);
}

module cavity() {
  translate([0, 0, 2]) {
    pcb_cavity();
    translate([50, 90, 12])
      button_cavity();
    translate([-10, 90, 12])
      indicator_light_cavity();
    translate([0, l, 0])
      battery_cavity();
  }
  translate([(w - 45) / 2, (l + 18) - 35, 0])
    coil_cavity();
  translate([w/2, box_l - 4, h - 10]) {
    nut();
    translate([-nut_radius, 0, 0])
      cube([nut_radius * 2, 5, nut_height]);
  }
  translate([w - w/4, 5, h - 10]) {
    nut();
    translate([-nut_radius, -6, 0])
      cube([nut_radius * 2, 5, nut_height]);
  }
}

difference() {
  cube([box_w, box_l, h]);
  translate([wall/2, 0, 1])
    cavity();
}

translate([box_w + 5, 0, 0]) {
  difference() {
    cube([box_w, box_l, wall]);
    translate([w/2, box_l - 4, 10]) {
      nut();
    }
    translate([w - w/4, 5, 10]) {
      nut();
    }
    translate([8, 40, 2])
    scale([0.2, 0.2, 0.2]) {
      linear_extrude(height = 20) import_dxf(file = "logo.dxf");
    }
  }
}

