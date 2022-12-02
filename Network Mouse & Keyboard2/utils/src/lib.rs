use std::cmp;
use std::ops;

#[derive(PartialEq, Debug, Clone, Copy)]
pub struct Coord(pub i32, pub i32);
pub type Id = usize;
pub type Resolution = (i32, i32);

pub fn get_current_resolution() -> Resolution {
    gdk::init();
    (gdk::Screen::width(), gdk::Screen::height())
}

#[derive(Clone, Copy, Debug)]
struct Cursor {
    x: i32,
    y: i32,
}

impl Cursor {
    fn new(coord: Coord) -> Self {
        Self {
            x: cmp::max(0, coord.0),
            y: cmp::max(0, coord.1),
        }
    }
    fn adjust_to(self, resolution: Resolution) -> Cursor {
        Cursor {
            x: cmp::min(self.x, resolution.0 - 1),
            y: cmp::min(self.y, resolution.1 - 1),
        }
    }

    fn coord(&self) -> Coord {
        Coord(self.x, self.y)
    }
}

pub struct Screen {
    pub id: Id,
    pub resolution: Resolution,
}

pub struct MultiScreen {
    screens: Vec<Screen>,
    main_screen_index: usize,
    cursor: Cursor,
}

impl MultiScreen {
    pub fn new(cursor: Coord, main_screen_id: Id, screens: Vec<Screen>) -> Self {
        if screens.is_empty() {
            panic!("expects non-empty screens");
        }
        let main_screen_index = screens
            .iter()
            .enumerate()
            .find(|(_, s)| s.id == main_screen_id)
            .expect("screens doesn't contain main id")
            .0;

        Self {
            screens,
            main_screen_index,
            cursor: Cursor::new(cursor),
        }
    }

    pub fn main_screen_id(&self) -> Id {
        self.get_main_screen().id
    }

    pub fn center_cursor(&mut self) -> Coord {
        let main_screen = self.get_main_screen();
        let center = Coord(main_screen.resolution.0 / 2, main_screen.resolution.1 / 2);
        self.cursor = Cursor::new(center);
        center
    }

    pub fn move_cursor_rel(&mut self, move_vector: Coord) -> (Id, Coord) {
        let mut new_coord = self.cursor.coord() + move_vector;

        let prev_main_screen_index = self.main_screen_index;

        if new_coord.0 < 0 {
            self.main_screen_index = self.prev_screen_index();
            if self.main_screen_index != prev_main_screen_index {
                new_coord.0 = cmp::max(new_coord.0, -self.get_main_screen().resolution.0)
                    + self.get_main_screen().resolution.0;
            }
        } else if new_coord.0 >= self.get_main_screen().resolution.0 {
            self.main_screen_index = self.next_screen_index();
            if self.main_screen_index != prev_main_screen_index {
                new_coord.0 -= self.get_main_screen().resolution.0;
            }
        }

        self.cursor = Cursor::new(new_coord).adjust_to(self.get_main_screen().resolution);

        (self.get_main_screen().id, self.cursor.coord())
    }

    pub fn move_cursor_abs(&mut self, new_position: Coord) -> Coord {
        let new_cursor = Cursor::new(new_position).adjust_to(self.get_main_screen().resolution);

        let difference = new_cursor.coord() - self.cursor.coord();
        let main_screen_index = self.main_screen_index;

        let (screen_id, final_position) = self.move_cursor_rel(difference);
        assert_eq!(main_screen_index, screen_id);
        assert_eq!(final_position, new_position);

        difference
    }

    fn get_main_screen(&self) -> &Screen {
        &self.screens[self.main_screen_index]
    }

    fn next_screen_index(&self) -> usize {
        cmp::min(self.main_screen_index + 1, self.screens.len() - 1)
    }

    fn prev_screen_index(&self) -> usize {
        if self.main_screen_index == 0 {
            return 0;
        }
        self.main_screen_index - 1
    }
}

impl ops::Add for Coord {
    type Output = Self;

    fn add(self, other: Self) -> Self::Output {
        Self(self.0 + other.0, self.1 + other.1)
    }
}

impl ops::Sub for Coord {
    type Output = Self;

    fn sub(self, other: Self) -> Self::Output {
        Self(self.0 - other.0, self.1 - other.1)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn create_one_screen(cursor: Coord, main_id: Id) -> MultiScreen {
        let main_screen = Screen {
            id: main_id,
            resolution: (100, 100),
        };

        MultiScreen::new(cursor, main_id, vec![main_screen])
    }

    #[test]
    fn create_multiscreen_with_one_screen() {
        create_one_screen(Coord(0, 0), 42);
    }

    #[test]
    #[should_panic]
    fn create_multiscreen_with_wrong_id_test() {
        let main_screen = Screen {
            id: 42,
            resolution: (100, 100),
        };

        MultiScreen::new(Coord(0, 0), 1, vec![main_screen]);
    }

    fn move_one_screen_helper_test(cursor: Coord, direction: Coord) {
        let main_id = 42;
        let mut multi_screen = create_one_screen(cursor, main_id);
        assert_eq!(multi_screen.move_cursor_rel(direction), (main_id, cursor));
    }

    #[test]
    fn move_cursor_to_left_one_screen_test() {
        move_one_screen_helper_test(Coord(0, 0), Coord(-100, 0));
    }

    #[test]
    fn move_cursor_up_one_screen_test() {
        move_one_screen_helper_test(Coord(0, 0), Coord(0, -100));
    }

    #[test]
    fn move_cursor_to_right_one_screen_test() {
        move_one_screen_helper_test(Coord(99, 0), Coord(100, 0));
    }

    #[test]
    fn move_cursor_down_one_screen_test() {
        move_one_screen_helper_test(Coord(99, 99), Coord(0, 100));
    }

    fn create_three_screen(cursor: Coord, main_id: Id) -> (Id, Id, MultiScreen) {
        let left_id = main_id + 10;
        let right_id = left_id + 10;
        let main_screen = Screen {
            id: main_id,
            resolution: (100, 100),
        };

        let left_screen = Screen {
            id: left_id,
            resolution: (100, 100),
        };

        let right_screen = Screen {
            id: right_id,
            resolution: (100, 100),
        };
        let multi_screen = MultiScreen::new(
            cursor,
            main_id,
            vec![left_screen, main_screen, right_screen],
        );

        (left_id, right_id, multi_screen)
    }

    #[test]
    fn move_cursor_inside_three_screen_test() {
        let main_id = 42;

        let (_, _, mut multi_screen) = create_three_screen(Coord(0, 0), main_id);
        assert_eq!(
            multi_screen.move_cursor_rel(Coord(1, 1)),
            (main_id, Coord(1, 1))
        );
    }

    #[test]
    fn move_cursor_to_left_three_screen_test() {
        let (left_screen_id, _, mut multi_screen) = create_three_screen(Coord(0, 0), 42);
        assert_eq!(
            multi_screen.move_cursor_rel(Coord(-1, 0)),
            (left_screen_id, Coord(99, 0))
        )
    }

    #[test]
    fn move_cursor_to_right_three_screen_test() {
        let (_, right_screen_id, mut multi_screen) = create_three_screen(Coord(99, 0), 42);
        assert_eq!(
            multi_screen.move_cursor_rel(Coord(1, 0)),
            (right_screen_id, Coord(0, 0))
        )
    }

    #[test]
    fn move_cursor_to_big_left_three_screen_test() {
        let (left_screen_id, _, mut multi_screen) = create_three_screen(Coord(0, 0), 42);
        assert_eq!(
            multi_screen.move_cursor_rel(Coord(-1000, 0)),
            (left_screen_id, Coord(0, 0))
        )
    }

    #[test]
    fn move_cursor_to_big_right_three_screen_test() {
        let (_, right_screen_id, mut multi_screen) = create_three_screen(Coord(99, 0), 42);
        assert_eq!(
            multi_screen.move_cursor_rel(Coord(100, 0)),
            (right_screen_id, Coord(99, 0))
        )
    }
}
