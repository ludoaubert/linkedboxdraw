use itertools::Itertools;
use std::cmp::Ordering;
use std::collections::HashMap;
use serde::{Serialize, Deserialize};
use std::env;
use std::ops::Sub;
use std::ops::Add;
use std::ops::AddAssign;
use std::f64::consts::PI;
use itertools::izip;
use std::collections::BTreeSet;
use std::collections::BTreeMap;
use std::cmp::min;
use std::cmp::max;

#[derive(Debug, PartialEq, Copy, Clone, Serialize, Deserialize, Eq, Ord, PartialOrd)]
struct Point{
    x: i32,
    y: i32
}
impl Sub for Point {
    type Output = Point;

    fn sub(self, rhs: Point) -> Point {
        Point {
            x: self.x - rhs.x,
            y: self.y - rhs.y,
        }
    }
}
impl Sub<&Point> for &Point {
    type Output = Point;

    fn sub(self, rhs: &Point) -> Point {
        Point {
            x: self.x - rhs.x,
            y: self.y - rhs.y,
        }
    }
}
impl Add for Point {
    type Output = Point;

    fn add(self, rhs: Point) -> Point {
        Point {
            x: self.x + rhs.x,
            y: self.y + rhs.y,
        }
    }
}
impl AddAssign for Point {
    fn add_assign(&mut self, rhs: Self) {
        self.x += rhs.x;
        self.y += rhs.y;
    }
}
#[derive(Clone, Serialize, Deserialize)]
struct Link{
    from: u32,
    to: u32,
    polyline: Vec<Point>
}
#[repr(u8)]
#[derive(Eq, Ord, Debug, PartialEq, PartialOrd, Copy, Clone, Serialize)]
enum RectangleEdge {
    Top = 0,
    Right = 1,
    Bottom = 2,
    Left = 3
}

impl RectangleEdge {
    fn rotate(self, steps: i32) -> Self {
        match (self as i32 + steps).rem_euclid(4) {
            0 => RectangleEdge::Top,
            1 => RectangleEdge::Right,
            2 => RectangleEdge::Bottom,
            3 => RectangleEdge::Left,
            _ => unreachable!(),
        }
    }
}

#[derive(Debug, PartialEq, Copy, Clone)]
enum PolylineDirection {
    Forward,
    Backward
}
#[derive(Clone)]
struct Rectangle {
    left:i32,
    right:i32,
    top:i32,
    bottom:i32
}
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Eq, Ord, PartialOrd)]
struct PointCoordinates {
    link_idx:usize,
    point_idx:usize,
    edge:Option<RectangleEdge>
}
#[derive(Debug, PartialEq, Serialize, Eq, Ord, PartialOrd, Clone)]
struct UpdateCommand {
    segment:(PointCoordinates, PointCoordinates),
    translation:Point
}
#[derive(Debug, Clone)]
struct ShallowLink<'a>{
    direction: PolylineDirection,
    from:u32,
    from_edge:RectangleEdge,
    to:u32,
    to_edge:RectangleEdge,
    polyline:Vec<&'a Point>
}
#[derive(Clone)]
struct State{
    lnks:Vec<Link>,
    crossings:u32
}
struct TestContext{
    rects:Vec<Rectangle>,
    lnks:Vec<Link>,
    update:BTreeSet<BTreeSet<UpdateCommand>>
}
enum SegmentDirection
{
    Up,
    Down,
    Left,
    Right
}

fn rotate_point(p: &Point, angle: f64) -> Point {
    let (s, c) = angle.sin_cos();
    Point {
        x : ((p.x as f64) * c - (p.y as f64) * s).round() as i32,
        y : ((p.x as f64) * s + (p.y as f64) * c).round() as i32
    }
}

fn rotate_rectangle(rec: &Rectangle, angle: f64) -> Rectangle {
    let p=rotate_point(&Point{x:rec.left,y:rec.top}, angle);
    let q=rotate_point(&Point{x:rec.right,y:rec.bottom}, angle);
    Rectangle{
        left:min(p.x, q.x),
        right:max(p.x, q.x),
        top:min(p.y, q.y),
        bottom:max(p.y, q.y)
    }
}

fn angle_to_steps(angle: f64) -> i32 {
    if angle == 0f64 {
        0
    } else if angle == PI / 2.0 {
        1
    } else if angle == -PI {
        2
    } else if angle == -PI / 2.0 {
        -1
    } else {
        println!("{}",angle);
        panic!("unsupported angle")
    }
}

fn is_between(p: Point, p1: Point, p2: Point) -> bool {
    // Cross product == 0 => collinear
    let cross =
        (p.y - p1.y) * (p2.x - p1.x)
      - (p.x - p1.x) * (p2.y - p1.y);

    if cross != 0 {
        return false;
    }

    p.x >= p1.x.min(p2.x)
        && p.x <= p1.x.max(p2.x)
        && p.y >= p1.y.min(p2.y)
        && p.y <= p1.y.max(p2.y)
}

fn untangle(lnks:&Vec<Link>)->BTreeSet<BTreeSet<UpdateCommand>>{

    let point_index: HashMap<*const Point, (usize, usize)> = lnks
        .iter()
        .enumerate()
        .flat_map(|(link_idx, lnk)| {
            lnk.polyline
                .iter()
                .enumerate()
                .map(move |(point_idx, p)| {
                    (p as *const Point, (link_idx, point_idx))
                })
        })
        .collect();
    
    let links : Vec<ShallowLink> = lnks
        .iter()
        .cartesian_product([PolylineDirection::Forward,PolylineDirection::Backward])
        .map(|(Link { from, to, polyline },dir)| -> (u32,u32,Vec<&Point>,PolylineDirection) {
            match dir {
                PolylineDirection::Forward => (*from, *to, polyline.iter().collect(), dir),
                PolylineDirection::Backward => (*to, *from, polyline.iter().rev().collect(), dir)
            }
        })
        .map(|(from,to,p,dir)| -> ShallowLink {
            let edge = |p0:&Point, p1:&Point| -> RectangleEdge {
                match (p0.x.cmp(&p1.x), p0.y.cmp(&p1.y)) {
                    (Ordering::Equal, Ordering::Greater) => RectangleEdge::Top,
                    (Ordering::Equal, Ordering::Less)    => RectangleEdge::Bottom,
                    (Ordering::Greater, Ordering::Equal) => RectangleEdge::Left,
                    (Ordering::Less, Ordering::Equal)    => RectangleEdge::Right,
                    _ => unreachable!("Polyline contains a non-axis-aligned segment")
                }
            };
            let [.., before_last, last] = p.as_slice() else { todo!()}; 
            ShallowLink{
                direction:dir,
                from:from,
                from_edge:edge(p[0],p[1]),
                to:to,
                to_edge:edge(last, before_last),
                polyline:p
            }
        })
        .sorted_by(|a, b| (a.from,a.from_edge).cmp(&(b.from,b.from_edge)))
        .collect();
    
    let update : BTreeSet<BTreeSet<UpdateCommand>> = links
        .iter()
        .chunk_by(|a| (a.from,a.from_edge))
        .into_iter()
        .map(|(key, group)| -> BTreeSet<UpdateCommand> {
            let (_from,from_edge) = key;
            let lnks_ : Vec<ShallowLink> = group.cloned().collect();
                
            println!("{:?}", lnks_);
            
            let angle = match from_edge {
                RectangleEdge::Right => 0f64,
                RectangleEdge::Left => PI,
                RectangleEdge::Top => PI / 2.0,
                RectangleEdge::Bottom => -PI / 2.0           
            };
        
            println!("angle:{:?}", angle);

//This returns:

//Ordering::Less → segment goes upward
//Ordering::Greater → segment goes downward
//Ordering::Equal → horizontal (which you say cannot happen)
  
            let link_order = |i: &usize, j: &usize| -> Ordering {
                let a = &lnks_[*i].polyline;
                let b = &lnks_[*j].polyline;
    
                let ord = match (a.len(), b.len()) {
                    (2, 2) => rotate_point(a[0],angle).y.cmp(&rotate_point(b[0],angle).y),
    
                    (2, _) => rotate_point(b[1],angle).y.cmp(&rotate_point(b[2],angle).y),
    
                    (_, 2) => rotate_point(a[1],angle).y.cmp(&rotate_point(a[2],angle).y),
    
                    _ => {
                    
                        let segment_direction = |p: &[&Point]| {
                            if rotate_point(p[2],angle).y > rotate_point(p[1],angle).y {
                                SegmentDirection::Down
                            } else {
                                SegmentDirection::Up
                            }
                        };
                        
                        let dir_a = segment_direction(a);
                        let dir_b = segment_direction(b);
    
                        match (dir_a, dir_b) {
                            (SegmentDirection::Up, SegmentDirection::Up) => rotate_point(a[1],angle).x.cmp(&rotate_point(b[1],angle).x),
                            (SegmentDirection::Down, SegmentDirection::Down) => rotate_point(b[1],angle).x.cmp(&rotate_point(a[1],angle).x),
                            (SegmentDirection::Up, SegmentDirection::Down) => Ordering::Less,
                            (SegmentDirection::Down, SegmentDirection::Up) => Ordering::Greater,
                            _ => unreachable!("Polyline contains a non-axis-aligned segment")
                        }
                    }
                };
                
                ord
            };
            
            let n: usize = lnks_.len();
            
            let v: Vec<usize> = (0..n)
                .sorted_by(link_order)
                .collect();
       
            println!("{:?}", v);
            
            let vv: Vec<usize> = (0..n)
                .sorted_by(|i, j| {
                    let a = &lnks_[*i].polyline;
                    let b = &lnks_[*j].polyline;
            
                    let ord = rotate_point(a[0], angle).y.cmp(&rotate_point(b[0], angle).y);
                    ord
                })
                .collect();
        
            for i in 0..n {
                println!("{} {} {}", i, v[i], vv[i]);
            }
    
            let update:BTreeSet<UpdateCommand> = izip!(v.iter(), vv.iter())
                .map(|(x, i)|{
                    let tr : Point = lnks_[*i].polyline[0] - lnks_[*x].polyline[0];
                    (x, tr)
                })
                .filter(|(_x,tr)|->bool {*tr != Point{x:0,y:0}})
                .map(|(x, tr)| {
                    let pc: Vec<PointCoordinates> = (0..2)
                        .map(|i| {
                            let p:&Point=lnks_[*x].polyline[i];
                            let key = p as *const Point;
                            let (link_idx, point_idx) = point_index[&key];
                            PointCoordinates{
                                link_idx,
                                point_idx,
                                edge:if i==0 {Some(lnks_[*x].from_edge)}
                                    else if i==lnks_[*x].polyline.len()-1 {Some(lnks_[*x].to_edge)}
                                    else {None}
                            }
                        })
                        .collect();
                    UpdateCommand{segment:(pc[0],pc[1]),translation:tr}
                })
                .collect();
    
            println!("{:?}", update);
            
            return update;
        })
        .filter(|update:&BTreeSet<UpdateCommand>| -> bool {update.is_empty()==false})
        .collect();
    
    return update;
}

fn filter(rects:&Vec<Rectangle>,
            lnks:&Vec<Link>,
            update:&BTreeSet<BTreeSet<UpdateCommand>>)->BTreeSet<BTreeSet<UpdateCommand>>{

    let filtered_update : BTreeSet<BTreeSet<UpdateCommand>> = update
        .iter()
        .filter(|v: &&BTreeSet<UpdateCommand>| {
            v.iter()
            .all(|&UpdateCommand {
                    segment: (
                        PointCoordinates { link_idx: l1, point_idx: idx1, edge: e1 },
                        PointCoordinates { link_idx: l2, point_idx: idx2, edge: e2 }
                    ),
                    translation: tr
                }| 
                {
                    let rec_edge=|edge:RectangleEdge, rec:&Rectangle|->(Point,Point){
                        let &Rectangle{left,right,top,bottom}=rec;
                        match edge{
                            RectangleEdge::Left => (Point{x:left,y:top},Point{x:left,y:bottom}),
                            RectangleEdge::Right => (Point{x:right,y:top},Point{x:right,y:bottom}),
                            RectangleEdge::Top => (Point{x:left,y:top},Point{x:right,y:top}),
                            RectangleEdge::Bottom => (Point{x:left,y:bottom},Point{x:right,y:bottom})
                        }
                    };
                    
                    let check = |link_idx, point_idx, edge: Option<RectangleEdge>| {
                        if let Some(edge) = edge {
                            let Link { from, to, polyline } = &lnks[link_idx];
                
                            let p = polyline[point_idx] + tr ;
                
                            let rec_idx = if point_idx == 0 {
                                *from
                            } else {
                                *to
                            } as usize;
                
                            let rec = &rects[rec_idx];
                
                            let (a, b) = rec_edge(edge, rec);
                
                            is_between(p, a, b)
                        } else {
                            true
                        }
                    };

                    check(l1, idx1, e1) && check(l2, idx2, e2)
                }
            )
        }).cloned()
        .collect();
    
    return filtered_update;
}

fn detect_crossings(polyline1: &Vec<Point>,
                    polyline2: &Vec<Point>)->u32
{
    let arr=[(polyline1,polyline2),(polyline2,polyline1)];
    
    arr.iter()
        .map(|(polyline1,polyline2)|{
    
            struct VerticalSegment {
                y_min: i32,
                y_max: i32,
                x: i32
            }
            
            struct HorizontalSegment {
                x_min: i32,
                x_max: i32,
                y: i32
            }
            
            let interval_index : BTreeMap<i32, Vec<VerticalSegment>> = 
                polyline2
                .iter()
                .tuple_windows()
                .filter(|(p1, p2)| p1.x == p2.x)
                .map(|(&p1, &p2)| VerticalSegment{
                    y_min:min(p1.y,p2.y),
                    y_max:max(p1.y,p2.y),
                    x:p1.x           
                }).into_group_map_by(|s| s.x)
                .into_iter()
                .collect();
                
            let crossings : u32 = polyline1
                .iter()
                .tuple_windows()
                .filter(|(p1, p2)| p1.y == p2.y)
                .map(|(&p1, &p2)| HorizontalSegment {
                    x_min: min(p1.x, p2.x),
                    x_max: max(p1.x, p2.x),
                    y: p1.y,
                })
                .map(|h| {
                    interval_index
                        .range(h.x_min..=h.x_max)
                        .map(|(_, verticals)| {
                            verticals
                                .iter()
                                .filter(|v| v.y_min <= h.y && h.y <= v.y_max)
                                .count()
                        })
                        .sum::<usize>()
                })
                .sum::<usize>() as u32;

            crossings
        }).sum()
}

fn detect_all_crossings(lnks:&Vec<Link>)->u32{
    lnks
        .iter()
        .map(|lnk| &lnk.polyline)
        .enumerate()
        .tuple_combinations()
        .filter(|((i, _p1), (j, _p2))| i<j)
        .map(|((_i, p1), (_j, p2))| {
            detect_crossings(p1, p2)
        }).sum()
}

fn apply(lnks:&Vec<Link>, update:&BTreeSet<BTreeSet<UpdateCommand>>)->Vec<Link>
{
    let current_state=State{lnks:lnks.clone(), crossings:detect_all_crossings(lnks)};
    println!("current_state.crossings={}", current_state.crossings);

    let apply_update=|state:State,update:&BTreeSet<UpdateCommand>| {
        let apply_uc=|mut state:State, &UpdateCommand{
                segment: (
                    PointCoordinates { link_idx: l1, point_idx: p1, edge: _e1 },
                    PointCoordinates { link_idx: l2, point_idx: p2, edge: _e2 },
                ),
                translation: tr
                }|->State
        {
            state.lnks[l1].polyline[p1] += tr;
            state.lnks[l2].polyline[p2] += tr;
            state
        };
    
        let next_state = update
            .iter()
            .fold(state.clone(), apply_uc);
        let crossings:u32 = detect_all_crossings(&next_state.lnks);
        if crossings < state.crossings {State{lnks:next_state.lnks,crossings:crossings}} else {state}
    };

    
    let final_state = update
        .iter()
        .fold(current_state, apply_update);

    println!("final_state.crossings={}", final_state.crossings);
    return final_state.lnks;
}

fn main() {

    env_logger::init();

    let args: Vec<String> = env::args().collect();

    for arg in &args {
        println!("{arg}");
    }
    
    if args.len()==2
    {
        let lnks: Vec<Link> = match serde_json::from_str(&args[1]) {
            Ok(v) => v,
            Err(e) => {
                eprintln!("Invalid JSON: {}", e);
                return;
            }
        };
        let update = untangle(&lnks);
        let json = serde_json::to_string(&update).unwrap();
        println!("{}", json);
        return;
    }
    
    let test_contexts : [TestContext;4]=[
        TestContext{
            rects:vec![Rectangle{left:10,right:40,top:20,bottom:70},Rectangle{left:180,right:210,top:30,bottom:80}],
            lnks:vec![
                Link{from:0,to:1,polyline:vec![Point{x:40,y:30},Point{x:90,y:30},Point{x:90,y:60},Point{x:180,y:60}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:50},Point{x:110,y:50},Point{x:110,y:70},Point{x:180,y:70}]}
            ],
/*
   10     40      90 110    180  210
  20+-----+
    |   30|-------+         +-----+
    |     |       |         |     |
    |   50|-------+--+      |  1  |
    |  0  |       +--+----->|60   |
  70+-----+          +----->|70   |
  80                        +-----+
*/
            update:BTreeSet::from([
                BTreeSet::from([UpdateCommand{segment:
                                    (PointCoordinates{link_idx:0,point_idx:0,edge:Some(RectangleEdge::Right)},
                                    PointCoordinates{link_idx:0,point_idx:1,edge:None}),
                                    translation:Point{x:0,y:20}},
                                UpdateCommand{segment:
                                    (PointCoordinates{link_idx:1,point_idx:0,edge:Some(RectangleEdge::Right)},
                                    PointCoordinates{link_idx:1,point_idx:1,edge:None}),
                                    translation:Point{x:0,y:-20}}]),
                BTreeSet::from([UpdateCommand{segment:
                                    (PointCoordinates{link_idx:0,point_idx:3,edge:Some(RectangleEdge::Left)},
                                    PointCoordinates{link_idx:0,point_idx:2,edge:None}),
                                    translation:Point{x:0,y:10}},
                                UpdateCommand{segment:
                                    (PointCoordinates{link_idx:1,point_idx:3,edge:Some(RectangleEdge::Left)},
                                    PointCoordinates{link_idx:1,point_idx:2,edge:None}),
                                    translation:Point{x:0,y:-10}}])
            ])

        },
        TestContext{
            rects:vec![Rectangle{left:10,right:40,top:40,bottom:80},Rectangle{left:160,right:190,top:30,bottom:70},Rectangle{left:160,right:190,top:80,bottom:140}],
            lnks:vec![
                Link{from:0,to:2,polyline:vec![Point{x:40,y:50},Point{x:110,y:50},Point{x:110,y:120},Point{x:160,y:120}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:60},Point{x:160,y:60}]}
            ],
/* 10     40     110       160   190
                            +-----+30
  40+-----+                 |     |
    |   50|-------+         |  1  |
    |   60|-------+-------->|     |
    |  0  |       |         +-----+70
  80+-----+       |         +-----+80
                  |         |     |
                  +-------->|120  |
                            |  2  |
                            +-----+140
*/
            update:BTreeSet::from([
                BTreeSet::from([UpdateCommand{segment:
                                    (PointCoordinates{link_idx:0,point_idx:0,edge:Some(RectangleEdge::Right)},
                                    PointCoordinates{link_idx:0,point_idx:1,edge:None}),
                                    translation:Point{x:0,y:10}},
                                UpdateCommand{segment:
                                    (PointCoordinates{link_idx:1,point_idx:0,edge:Some(RectangleEdge::Right)},
                                    PointCoordinates{link_idx:1,point_idx:1,edge:Some(RectangleEdge::Left)}),
                                    translation:Point{x:0,y:-10}}
                ])
            ])
        },
        TestContext{
            rects:vec![Rectangle{left:10,right:40,top:40,bottom:90},Rectangle{left:160,right:190,top:60,bottom:100},Rectangle{left:160,right:190,top:110,bottom:150}],
            lnks:vec![
                Link{from:0,to:2,polyline:vec![Point{x:40,y:50},Point{x:110,y:50},Point{x:110,y:120},Point{x:160,y:120}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:70},Point{x:160,y:70}]}
            ],
/*
  10      40     110       160   190
  40+-----+
    |   50|-------+
    |  0  |       |         +-----+60
    |   70|-------+-------->|70   |
    |     |       |         |  1  |
  90+-----+       |         |     |
                  |         +-----+100
                  |         +-----+110
                  +-------->|120  |
                            |  2  |
                            |     |
                            +-----+150
*/
            update:BTreeSet::from([
                BTreeSet::from([UpdateCommand{segment:
                                    (PointCoordinates{link_idx:0,point_idx:0,edge:Some(RectangleEdge::Right)},
                                    PointCoordinates{link_idx:0,point_idx:1,edge:None}),
                                    translation:Point{x:0,y:20}},
                                UpdateCommand{segment:
                                    (PointCoordinates{link_idx:1,point_idx:0,edge:Some(RectangleEdge::Right)},
                                    PointCoordinates{link_idx:1,point_idx:1,edge:Some(RectangleEdge::Left)}),
                                    translation:Point{x:0,y:-20}}
                ])
            ])
        },
        TestContext{
            rects:vec![Rectangle{left:10,right:40,top:170,bottom:220},Rectangle{left:140,right:170,top:20,bottom:70}],
            lnks:vec![
                Link{from:0,to:1,polyline:vec![Point{x:40,y:180},Point{x:100,y:180},Point{x:100,y:30},Point{x:120,y:30}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:190},Point{x:90,y:190},Point{x:90,y:40},Point{x:120,y:40}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:200},Point{x:80,y:200},Point{x:80,y:50},Point{x:120,y:50}]}
            ],
/*                           140   170
                              +-----+20
                        +---->|30   |
                     +--+---->|40   |
                  +--+--+---->|50   |
                  |  |  |     |  1  |
   10    40       |  |  |     +-----+70
 170+-----+       |  |  |
    |  180|-------+--+--+
    |  190|-------+--+      
    |  200|-------+
    |  0  |
 220+-----+      80 90 100
*/
            update:BTreeSet::from([
                BTreeSet::from([UpdateCommand{segment:
                                    (PointCoordinates{link_idx:0,point_idx:0,edge:Some(RectangleEdge::Right)},
                                    PointCoordinates{link_idx:0,point_idx:1,edge:None}),
                                    translation:Point{x:0,y:20}},
                                UpdateCommand{segment:
                                    (PointCoordinates{link_idx:2,point_idx:0,edge:Some(RectangleEdge::Right)},
                                    PointCoordinates{link_idx:2,point_idx:1,edge:None}),
                                    translation:Point{x:0,y:-20}}]),
                BTreeSet::from([UpdateCommand{segment:
                                    (PointCoordinates{link_idx:0,point_idx:3,edge:Some(RectangleEdge::Left)},
                                    PointCoordinates{link_idx:0,point_idx:2,edge:None}),
                                    translation:Point{x:0,y:20}},
                                UpdateCommand{segment:
                                    (PointCoordinates{link_idx:2,point_idx:3,edge:Some(RectangleEdge::Left)},
                                    PointCoordinates{link_idx:2,point_idx:2,edge:None}),
                                    translation:Point{x:0,y:-20}}])
            ])
        }
    ];

    let angles:[f64;4]=[0f64, -PI, -PI / 2.0, PI / 2.0];
    
    let synthetic_test_contexts : Vec<TestContext> = test_contexts
        .iter()
        .cartesian_product(angles)
        .map(|(ctx,angle)| {
            TestContext{
                rects:ctx.rects
                        .iter()
                        .map(|rec| rotate_rectangle(rec, angle))
                        .collect(),
                lnks:ctx.lnks
                    .iter()
                    .map(|lnk:&Link| Link{
                        from:lnk.from,
                        to:lnk.to,
                        polyline:lnk.polyline
                            .iter()
                            .map(|p| rotate_point(p, angle))
                            .collect(),
                    })
                    .collect(),
                update:ctx.update
                    .iter()
                    .map(|v:&BTreeSet<UpdateCommand>|
                            v
                            .iter()
                            .map(|uc:&UpdateCommand| {
                                let (pc1, pc2) = uc.segment;
                                //let (PointCoordinates{link_idx:0,point_idx:0,edge:Some(RectangleEdge::Right)})
                                UpdateCommand{
                                    segment:(PointCoordinates{
                                                link_idx:pc1.link_idx,
                                                point_idx:pc1.point_idx,
                                                edge:pc1.edge.map(|e| e.rotate(angle_to_steps(angle)))},
                                            PointCoordinates{
                                                link_idx:pc2.link_idx,
                                                point_idx:pc2.point_idx,
                                                edge:pc2.edge.map(|e| e.rotate(angle_to_steps(angle)))}),
                                    translation:rotate_point(&uc.translation,angle)
                                }
                            })
                            .collect()
                        )
                        .collect()
                }
            })
            .collect();
            
    let mut nb_ok:u32=0;
    let mut nb_ko:u32=0;

    for TestContext { rects, lnks, update: expected } in &synthetic_test_contexts {
        let update = untangle(&lnks);
        let filtered_update = filter(&rects, &lnks, &update);

        let uncrossed_lnks = apply(lnks, &filtered_update);
    
        println!("{:?}", update);
        let json = serde_json::to_string(&update).unwrap();
        println!("{}", json);
        let json_output = serde_json::to_string(&uncrossed_lnks).unwrap();
        println!("{}", json_output);
        let b:bool = update==*expected;
        let status : &str = if b {"OK"} else {"KO"};
        println!("{}", status);
        if b{
            nb_ok += 1;
        }else{
            nb_ko += 1;
        }
    }
    println!("{}/{} tests OK.", nb_ok, nb_ok + nb_ko);
}
