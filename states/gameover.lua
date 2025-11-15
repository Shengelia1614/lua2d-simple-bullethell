local GameOverState = {}

local VIRTUAL_WIDTH, VIRTUAL_HEIGHT = 1280, 720

function GameOverState:enter()
end

function GameOverState:update(dt)
    if love.keyboard.isDown('space') or love.keyboard.isDown('return') then
        return 'menu'
    end
    return nil
end

function GameOverState:draw()
    love.graphics.setColor(1, 0, 0)
    love.graphics.printf('GAME OVER', 0, VIRTUAL_HEIGHT / 2 - 40, VIRTUAL_WIDTH, 'center')
    love.graphics.setColor(1, 1, 1)
    love.graphics.printf('Press SPACE or ENTER to return to menu', 0, VIRTUAL_HEIGHT / 2, VIRTUAL_WIDTH, 'center')
end

function GameOverState:exit()
end

return GameOverState
