#include <entt/entt.hpp>

#include <fmt/core.h>
#include <iostream>

struct IntComp
{
    int i;
};

struct CharComp
{
    char c;
};

int main()
{
    entt::registry reg;

    {
        const auto ent = reg.create();
        reg.emplace<IntComp>(ent, 1);
        reg.emplace<CharComp>(ent, 'z');
    }
    {
        const auto ent = reg.create();
        reg.emplace<IntComp>(ent, 2);
        reg.emplace<CharComp>(ent, 'y');
    }
    {
        const auto ent = reg.create();
        reg.emplace<IntComp>(ent, 3);
        reg.emplace<CharComp>(ent, 'x');
    }
    {
        const auto ent = reg.create();
        reg.emplace<IntComp>(ent, 4);
        reg.emplace<CharComp>(ent, 'w');
    }
    {
        const auto ent = reg.create();
        reg.emplace<IntComp>(ent, 5);
        reg.emplace<CharComp>(ent, 'v');
    }

    reg.sort<IntComp>([](const IntComp& lhs, const IntComp& rhs) { return lhs.i < rhs.i; });
    reg.sort<CharComp, IntComp>();

    {
        const auto v = reg.view<IntComp>();
        for (auto [ent, ic] : v.each())
        {
            fmt::print("{} ", ic.i);
            std::cout << '\n';
        }
    }
    {
        const auto v = reg.view<CharComp>();
        for (auto [ent, cc] : v.each())
        {
            fmt::print("{} ", cc.c);
            std::cout << '\n';
        }
    }

    return 0;
}