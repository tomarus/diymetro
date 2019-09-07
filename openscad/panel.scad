panelThickness = 2;
panelHp=34;
holeCount=4;
holeWidth = 5.08; //If you want wider holes for easier mounting. Otherwise set to any number lower than mountHoleDiameter. Can be passed in as parameter to eurorackPanel()


threeUHeight = 133.35; //overall 3u height
panelOuterHeight =128.5;
panelInnerHeight = 110; //rail clearance = ~11.675mm, top and bottom
railHeight = (threeUHeight-panelOuterHeight)/2;
mountSurfaceHeight = (panelOuterHeight-panelInnerHeight-railHeight*2)/2;

echo("mountSurfaceHeight",mountSurfaceHeight);

hp=5.08;
mountHoleDiameter = 3.2;
mountHoleRad =mountHoleDiameter/2;
hwCubeWidth = holeWidth-mountHoleDiameter;

offsetToMountHoleCenterY=mountSurfaceHeight/2;
offsetToMountHoleCenterX = hp - hwCubeWidth/2; // 1 hp from side to center of hole

echo(offsetToMountHoleCenterY);
echo(offsetToMountHoleCenterX);

module eurorackPanel(panelHp,  mountHoles=2, hw = holeWidth, ignoreMountHoles=false)
{
    //mountHoles ought to be even. Odd values are -=1
    difference()
    {
        cube([hp*panelHp,panelOuterHeight,panelThickness]);
        
        if(!ignoreMountHoles)
        {
            eurorackMountHoles(panelHp, mountHoles, holeWidth);
        }
    }
}

module eurorackMountHoles(php, holes, hw)
{
    holes = holes-holes%2;//mountHoles ought to be even for the sake of code complexity. Odd values are -=1
    eurorackMountHolesTopRow(php, hw, holes/2);
    eurorackMountHolesBottomRow(php, hw, holes/2);
}

module eurorackMountHolesTopRow(php, hw, holes)
{
    
    //topleft
    translate([offsetToMountHoleCenterX,panelOuterHeight-offsetToMountHoleCenterY,0])
    {
        eurorackMountHole(hw);
    }
    if(holes>1)
    {
        translate([(hp*php)-hwCubeWidth-hp,panelOuterHeight-offsetToMountHoleCenterY,0])
    {
        eurorackMountHole(hw);
    }
    }
    if(holes>2)
    {
        holeDivs = php*hp/(holes-1);
        for (i =[1:holes-2])
        {
            translate([holeDivs*i,panelOuterHeight-offsetToMountHoleCenterY,0]){
                eurorackMountHole(hw);
            }
        }
    }
}

module eurorackMountHolesBottomRow(php, hw, holes)
{
    
    //bottomRight
    translate([(hp*php)-hwCubeWidth-hp,offsetToMountHoleCenterY,0])
    {
        eurorackMountHole(hw);
    }
    if(holes>1)
    {
        translate([offsetToMountHoleCenterX,offsetToMountHoleCenterY,0])
    {
        eurorackMountHole(hw);
    }
    }
    if(holes>2)
    {
        holeDivs = php*hp/(holes-1);
        for (i =[1:holes-2])
        {
            translate([holeDivs*i,offsetToMountHoleCenterY,0]){
                eurorackMountHole(hw);
            }
        }
    }
}

module eurorackMountHole(hw)
{
    
    mountHoleDepth = panelThickness+2; //because diffs need to be larger than the object they are being diffed from for ideal BSP operations
    
    if(hwCubeWidth<0)
    {
        hwCubeWidth=0;
    }
    translate([0,0,-1]){
    union()
    {
        cylinder(r=mountHoleRad, h=mountHoleDepth, $fn=20);
        translate([0,-mountHoleRad,0]){
        cube([hwCubeWidth, mountHoleDiameter, mountHoleDepth]);
        }
        translate([hwCubeWidth,0,0]){
            cylinder(r=mountHoleRad, h=mountHoleDepth, $fn=20);
            }
    }
}
}


module MidiPlug(x, y, h) {
    translate([x, y,-1])  cylinder(d=15.5, h=h, $fn=50); 
    translate([x, y-11,-1])  cylinder(d=3.3, h=h, $fn=20); 
    translate([x, y+11,-1])  cylinder(d=3.3, h=h, $fn=20); 
}

module Potmeter(x, y, h) {
    translate([x, y,-1])  cylinder(d=6.4, h=h, $fn=100); 
}

module Led(x, y, h) {
    translate([x, y,-1])  cylinder(d=3.2, h=h, $fn=25); 
}

module Hole(x, y, h) {
    translate([x,y,-1]) cylinder(d=1, h=h, $fn=4);
}

module colum(x, h) {
potrowh = panelInnerHeight / 5;
off = ((panelOuterHeight - panelInnerHeight)/2) - (potrowh/2);

Potmeter(x, off + (1*potrowh)+5, h);
Potmeter(x, off + (2*potrowh)+5, h);
Potmeter(x, off + (3*potrowh)+5, h);
Potmeter(x, off + (4*potrowh)+5, h);
Led(x, off + (5*potrowh), h);
}

module columb(x, h) {
potrowh = panelInnerHeight / 5;
off = ((panelOuterHeight - panelInnerHeight)/2) - (potrowh/2);

Hole(x, off + (1*potrowh)+5, h);
Hole(x, off + (2*potrowh)+5, h);
Hole(x, off + (3*potrowh)+5, h);
Hole(x, off + (4*potrowh)+5, h);
Hole(x, off + (5*potrowh), h);
}

w = hp*panelHp;

projection() translate([0,0,0]) rotate([0,0,0])
difference() {
eurorackPanel(panelHp, holeCount, holeWidth);
mountHoleDepth = panelThickness+2;

ss = 16.36;
columb(ss+(0*20), mountHoleDepth);
columb(ss+(1*20), mountHoleDepth);
columb(ss+(2*20), mountHoleDepth);
columb(ss+(3*20), mountHoleDepth);
columb(ss+(4*20), mountHoleDepth);
columb(ss+(5*20), mountHoleDepth);
columb(ss+(6*20), mountHoleDepth);
columb(ss+(7*20), mountHoleDepth);
}

