local GameOverState = {}

function GameOverState:enter()
end

function GameOverState:update(dt)
    if love.keyboard.isDown('space') or love.keyboard.isDown('return') then
        return 'playing'
    end
    return nil
end

function GameOverState:draw()
    love.graphics.setColor(1, 0, 0)
    love.graphics.printf('GAME OVER', 0, love.graphics.getHeight() / 2 - 40, love.graphics.getWidth(), 'center')
    love.graphics.setColor(1, 1, 1)
    love.graphics.printf('Press SPACE or ENTER to restart', 0, love.graphics.getHeight() / 2, love.graphics.getWidth(),
        'center')
end

function GameOverState:exit()
end

return GameOverState
